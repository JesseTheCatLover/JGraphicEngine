// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Framework/AssetManager.h"

#include <algorithm>
#include <iostream>

#include "Assets/AssetFile.h"
#include "Assets/AssetImportSubsystem.h"
#include "Assets/AssetRegistrySubsystem.h"
#include "Assets/AssetRegistryScanner.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UUUID.h"

namespace
{
    // "/Project/Folder/Asset.jasset" -> "/Project/Folder"
    std::string GetParentFolder(const std::string& virtualPath)
    {
        const std::size_t pos = virtualPath.find_last_of('/');
        if (pos == std::string::npos || pos == 0)
            return virtualPath; // "/Project" or malformed, just return as-is

        return virtualPath.substr(0, pos);
    }
}

bool AssetManager::Initialize(AssetRegistrySubsystem* registry,
                              AssetImportSubsystem* importer,
                              VirtualPathMounter* pathMounter)
{
    m_Registry = registry;
    m_Importer = importer;
    m_PathMounter = pathMounter;

    return m_Registry != nullptr &&
           m_Importer != nullptr &&
           m_PathMounter != nullptr;
}

void AssetManager::Shutdown()
{
    m_Registry = nullptr;
    m_Importer = nullptr;
    m_PathMounter = nullptr;
}

FAssetOpResult AssetManager::InitialSyncRegistryFromDisk()
{
    if (bInitialSynced)
        return {};

    FAssetOpResult result;

    if (!m_Registry || !m_PathMounter)
    {
        result.bSuccess = false;
        result.errors.emplace_back("AssetManager not initialized (Registry/PathMounter is null).");
        return result;
    }

    // Decide which mounted virtual roots we want to sync at boot
    const std::vector<std::string> rootsToSync = {
        "/Engine",
        "/Project",
    };

    bool bAnyHardFailure = false;

    for (const std::string& root : rootsToSync)
    {
        FAssetOpResult opResult = SyncRootToRegistry(root);

        // Merge affected folders
        for (const auto& v : opResult.affectedVirtualFolders)
            result.affectedVirtualFolders.push_back(v);

        // Merge diagnostics
        result.warnings.insert(result.warnings.end(), opResult.warnings.begin(), opResult.warnings.end());
        result.errors.insert(result.errors.end(), opResult.errors.begin(), opResult.errors.end());

        if (!opResult.bSuccess)
            bAnyHardFailure = true;
    }

    result.bSuccess = !bAnyHardFailure;
    bInitialSynced = true;
    return result;
}

bool AssetManager::ImportAsset(const FAssetImportRequest& request, FAssetImportResult& outResult)
{
    outResult = {};

    if (!m_Importer || !m_Registry || !m_PathMounter)
    {
        std::cerr << "[AssetManager]: Asset manager is not initialized\n";
        outResult.bSuccess = false;
        return false;
    }

    if (!m_Importer->Import(request, *m_PathMounter, outResult))
    {
        // Import subsystem should have filled errors/warnings appropriately.
        outResult.bSuccess = false;
        return false;
    }

    // Rebuild only the folders that contain the imported assets.
    std::vector<std::string> foldersToSync;
    foldersToSync.reserve(outResult.createdAssets.size());

    for (const FImportedAssetInfo& info : outResult.createdAssets)
    {
        if (info.virtualPath.empty())
            continue;

        const std::string folderVirtualPath = GetParentFolder(info.virtualPath);

        auto it = std::find(foldersToSync.begin(), foldersToSync.end(), folderVirtualPath);
        if (it == foldersToSync.end())
            foldersToSync.push_back(folderVirtualPath);
    }

    if (!foldersToSync.empty())
    {
        // Fallback: if no createdAssets were returned, refresh /Project
        FAssetOpResult sync = SyncRootToRegistry("/Project");
        if (!sync.bSuccess)
            outResult.warnings.emplace_back("Import succeeded, but /Project registry sync had issues.");
        outResult.bSuccess = outResult.errors.empty();
        return outResult.bSuccess;
    }

    for (const std::string& folder : foldersToSync)
    {
        FAssetOpResult sync = SyncFolderToRegistry(folder);
        if (!sync.bSuccess)
        {
            outResult.warnings.emplace_back("Import succeeded, but registry sync failed for: " + folder);
            for (const std::string& e : sync.errors) outResult.warnings.emplace_back("  " + e);
        }
        else
        {
            for (const std::string& w : sync.warnings) outResult.warnings.emplace_back(w);
        }
    }

    outResult.bSuccess = true;
    return true;
}

const FAssetRecord* AssetManager::FindAssetByAssetID(const std::string& assetID) const
{
    return m_Registry ? m_Registry->FindAssetByAssetID(assetID) : nullptr;
}

const FAssetRecord* AssetManager::FindAssetByVirtualPath(const std::string& virtualPath) const
{
    return m_Registry ? m_Registry->FindAssetByVirtualPath(virtualPath) : nullptr;
}

std::vector<const FAssetRecord*> AssetManager::FindAllAssetsByVirtualPathPrefix(const std::string& virtualPathPrefix) const
{
    // TIGHTENED: Returned {} instead of {nullptr} to prevent crashes in iterators
    return m_Registry ? m_Registry->FindAllAssetsByVirtualPathPrefix(virtualPathPrefix) : std::vector<const FAssetRecord*>{};
}

const FAssetRecord* AssetManager::FindAssetByPhysicalPath(const std::string& physicalPath) const
{
    return m_Registry ? m_Registry->FindAssetByPhysicalPath(physicalPath) : nullptr;
}

const std::vector<FAssetRecord>* AssetManager::GetAllAssets() const
{
    return m_Registry ? &m_Registry->GetAllAssets() : nullptr;
}

std::vector<const FAssetRecord*> AssetManager::GetAllUserVisibleAssets() const
{
    if (!m_Registry) { std::cerr << "[AssetManager]: Registry is null\n"; return {}; }
    return m_Registry->GetAllUserVisibleAssets();
}

std::vector<const FAssetRecord*> AssetManager::GetAllAssetsByType(EAssetType type) const
{
    if (!m_Registry) { std::cerr << "[AssetManager]: Registry is null\n"; return {}; }
    return m_Registry->GetAllAssetsByType(type);
}

std::vector<const FAssetRecord*> AssetManager::GetAllAssetsByDomain(EAssetDomain domain) const
{
    if (!m_Registry) { std::cerr << "[AssetManager]: Registry is null\n"; return {}; }
    return m_Registry->GetAllAssetsByDomain(domain);
}

std::vector<const FAssetRecord*> AssetManager::GetAllAssetsByVisibility(EAssetVisibility visibility) const
{
    if (!m_Registry) { std::cerr << "[AssetManager]: Registry is null\n"; return {}; }
    return m_Registry->GetAllAssetsByVisibility(visibility);
}

std::vector<const FAssetRecord*> AssetManager::GetAllDependenciesForAsset(const std::string& assetID) const
{
    if (!m_Registry) { std::cerr << "[AssetManager]: Registry is null\n"; return {}; }
    return m_Registry->GetAllDependenciesForAsset(assetID);
}

std::vector<std::string> AssetManager::ListFolders(const std::string& parentVirtualFolder, bool bRecursive) const
{
    std::vector<std::string> result;

    if (!m_PathMounter)
        return result;

    std::string parentV = NormalizeVirtualFolder(parentVirtualFolder);

    std::string physicalParent;
    if (!ResolveVirtualToPhysical(parentV, physicalParent))
        return result;

    std::vector<std::string> physicalDirs = UFileSystem::ListDirectories(physicalParent, bRecursive);

    result.reserve(physicalDirs.size());
    for (const std::string& physDir : physicalDirs)
    {
        std::string virtualDir;
        if (m_PathMounter->ResolvePhysicalToVirtual(physDir, virtualDir))
            result.push_back(NormalizeVirtualFolder(virtualDir));
    }

    return result;
}
// =====================================================================
// Folder operations (collision-safe)
// =====================================================================

FAssetOpResult AssetManager::CreateFolder(const std::string& folderVirtualPath)
{
    FAssetOpResult r;

    if (!m_PathMounter)
    {
        r.errors.emplace_back("AssetManager not initialized (PathMounter is null).");
        return r;
    }

    std::string v = NormalizeVirtualFolder(folderVirtualPath);

    if (!IsWritableVirtualPath(v))
    {
        r.errors.emplace_back("Path is not writable: " + v);
        return r;
    }

    bool bExists = false;
    if (!VirtualFolderExistsOnDisk(v, bExists))
    {
        r.errors.emplace_back("Failed to resolve folder path: " + v);
        return r;
    }
    if (bExists)
    {
        r.errors.emplace_back("Folder already exists: " + v);
        return r;
    }

    std::string physicalPath;
    if (!ResolveVirtualToPhysical(v, physicalPath))
    {
        r.errors.emplace_back("Failed to resolve folder path: " + v);
        return r;
    }

    if (!UFileSystem::CreateDirectory(physicalPath))
    {
        r.errors.emplace_back("Failed to create directory: " + physicalPath);
        return r;
    }

    r.bSuccess = true;
    return r;
}

FAssetOpResult AssetManager::DeleteFolder(const std::string& folderVirtualPath, bool bRecursive)
{
    FAssetOpResult r;

    if (!m_PathMounter || !m_Registry)
    {
        r.errors.emplace_back("AssetManager not initialized (Registry/PathMounter is null).");
        return r;
    }

    std::string v = NormalizeVirtualFolder(folderVirtualPath);

    if (!IsWritableVirtualPath(v))
    {
        r.errors.emplace_back("Path is not writable: " + v);
        return r;
    }

    std::string physicalPath;
    if (!ResolveVirtualToPhysical(v, physicalPath))
    {
        r.errors.emplace_back("Failed to resolve folder path: " + v);
        return r;
    }

    // Optional: If folder doesn't exist, return error (strict) instead of success.
    // Professional editors usually error.
    if (!UFileSystem::DirectoryExists(physicalPath))
    {
        r.errors.emplace_back("Folder does not exist: " + v);
        return r;
    }

    if (!UFileSystem::DeleteDirectory(physicalPath, bRecursive))
    {
        r.errors.emplace_back("Failed to delete directory: " + physicalPath);
        return r;
    }

    // Sync the deleted folder prefix to purge assets in that subtree.
    FAssetOpResult sync = SyncFolderToRegistry(v);
    r.affectedVirtualFolders = sync.affectedVirtualFolders;

    if (!sync.bSuccess)
    {
        r.errors.insert(r.errors.end(), sync.errors.begin(), sync.errors.end());
        r.warnings.insert(r.warnings.end(), sync.warnings.begin(), sync.warnings.end());
        r.bSuccess = false;
        return r;
    }

    r.warnings.insert(r.warnings.end(), sync.warnings.begin(), sync.warnings.end());
    r.bSuccess = true;
    return r;
}

FAssetOpResult AssetManager::RenameFolder(const std::string& oldVirtualPath, const std::string& newVirtualPath)
{
    FAssetOpResult r;

    if (!m_PathMounter || !m_Registry)
    {
        r.errors.emplace_back("AssetManager not initialized (Registry/PathMounter is null).");
        return r;
    }

    std::string oldV = NormalizeVirtualFolder(oldVirtualPath);
    std::string newV = NormalizeVirtualFolder(newVirtualPath);

    if (!IsWritableVirtualPath(oldV) || !IsWritableVirtualPath(newV))
    {
        r.errors.emplace_back("Rename must stay inside writable mount: " + oldV + " -> " + newV);
        return r;
    }

    bool bDestExists = false;
    if (!VirtualFolderExistsOnDisk(newV, bDestExists))
    {
        r.errors.emplace_back("Failed to resolve destination folder path: " + newV);
        return r;
    }
    if (bDestExists)
    {
        r.errors.emplace_back("Destination folder already exists: " + newV);
        return r;
    }

    std::string oldPhysical, newPhysical;
    if (!ResolveVirtualToPhysical(oldV, oldPhysical) || !ResolveVirtualToPhysical(newV, newPhysical))
    {
        r.errors.emplace_back("Failed to resolve folder paths.");
        return r;
    }

    if (!UFileSystem::DirectoryExists(oldPhysical))
    {
        r.errors.emplace_back("Source folder does not exist: " + oldV);
        return r;
    }

    if (!UFileSystem::MoveDirectory(oldPhysical, newPhysical))
    {
        r.errors.emplace_back("Failed to move directory: " + oldPhysical + " -> " + newPhysical);
        return r;
    }

    // Sync both: old clears, new populates
    FAssetOpResult syncOld = SyncFolderToRegistry(oldV);
    FAssetOpResult syncNew = SyncFolderToRegistry(newV);

    r.affectedVirtualFolders.insert(r.affectedVirtualFolders.end(),
                                   syncOld.affectedVirtualFolders.begin(), syncOld.affectedVirtualFolders.end());
    r.affectedVirtualFolders.insert(r.affectedVirtualFolders.end(),
                                   syncNew.affectedVirtualFolders.begin(), syncNew.affectedVirtualFolders.end());

    // If either registry update failed, consider operation failed (disk already moved, but registry must reflect truth).
    if (!syncOld.bSuccess || !syncNew.bSuccess)
    {
        r.errors.emplace_back("Registry sync failed during folder rename.");
        r.errors.insert(r.errors.end(), syncOld.errors.begin(), syncOld.errors.end());
        r.errors.insert(r.errors.end(), syncNew.errors.begin(), syncNew.errors.end());
        r.warnings.insert(r.warnings.end(), syncOld.warnings.begin(), syncOld.warnings.end());
        r.warnings.insert(r.warnings.end(), syncNew.warnings.begin(), syncNew.warnings.end());
        r.bSuccess = false;
        return r;
    }

    r.warnings.insert(r.warnings.end(), syncOld.warnings.begin(), syncOld.warnings.end());
    r.warnings.insert(r.warnings.end(), syncNew.warnings.begin(), syncNew.warnings.end());
    r.bSuccess = true;
    return r;
}

FAssetOpResult AssetManager::MoveFolder(const std::string& sourceVirtualPath, const std::string& destVirtualPath)
{
    // Same implementation as rename at the disk level
    return RenameFolder(sourceVirtualPath, destVirtualPath);
}

// =====================================================================
// Asset operations (collision-safe)
// =====================================================================

FAssetOpResult AssetManager::DeleteAsset(const std::string& virtualAssetPath)
{
    FAssetOpResult r;

    if (!m_PathMounter || !m_Registry)
    {
        r.errors.emplace_back("AssetManager not initialized (Registry/PathMounter is null).");
        return r;
    }

    std::string v = NormalizeVirtualPath(virtualAssetPath);

    if (!IsWritableVirtualPath(v))
    {
        r.errors.emplace_back("Path is not writable: " + v);
        return r;
    }

    if (!EndsWith(v, ".jasset"))
    {
        r.errors.emplace_back("Not an asset file (.jasset): " + v);
        return r;
    }

    std::string physical;
    if (!ResolveVirtualToPhysical(v, physical))
    {
        r.errors.emplace_back("Failed to resolve asset path: " + v);
        return r;
    }

    if (!UFileSystem::FileExists(physical))
    {
        r.errors.emplace_back("Asset file does not exist: " + v);
        return r;
    }

    if (!UFileSystem::DeleteFile(physical))
    {
        r.errors.emplace_back("Failed to delete asset file: " + physical);
        return r;
    }

    // Sync parent folder
    const std::string parent = NormalizeVirtualFolder(GetParentFolder(v));
    FAssetOpResult sync = SyncFolderToRegistry(parent);

    r.affectedVirtualFolders = sync.affectedVirtualFolders;
    if (!sync.bSuccess)
    {
        r.errors.insert(r.errors.end(), sync.errors.begin(), sync.errors.end());
        r.warnings.insert(r.warnings.end(), sync.warnings.begin(), sync.warnings.end());
        r.bSuccess = false;
        return r;
    }

    r.warnings.insert(r.warnings.end(), sync.warnings.begin(), sync.warnings.end());
    r.bSuccess = true;
    return r;
}

FAssetOpResult AssetManager::RenameAsset(const std::string& virtualAssetPath, const std::string& newName)
{
    FAssetOpResult r;

    if (!m_PathMounter || !m_Registry)
    {
        r.errors.emplace_back("AssetManager not initialized (Registry/PathMounter is null).");
        return r;
    }

    std::string srcV = NormalizeVirtualPath(virtualAssetPath);

    if (!IsWritableVirtualPath(srcV))
    {
        r.errors.emplace_back("Path is not writable: " + srcV);
        return r;
    }

    if (!EndsWith(srcV, ".jasset"))
    {
        r.errors.emplace_back("Not an asset file (.jasset): " + srcV);
        return r;
    }

    if (!IsSimpleName(newName))
    {
        r.errors.emplace_back("Invalid asset name: " + newName);
        return r;
    }

    std::string leaf = newName;
    if (!EndsWith(leaf, ".jasset"))
        leaf += ".jasset";

    const std::string parent = NormalizeVirtualFolder(GetParentFolder(srcV));
    const std::string dstV = JoinVirtual(parent, leaf);

    // Collision check
    bool bDestExists = false;
    if (!VirtualFileExistsOnDisk(dstV, bDestExists))
    {
        r.errors.emplace_back("Failed to resolve destination path: " + dstV);
        return r;
    }
    if (bDestExists)
    {
        r.errors.emplace_back("Name collision: destination asset already exists: " + dstV);
        return r;
    }

    std::string srcP, dstP;
    if (!ResolveVirtualToPhysical(srcV, srcP) || !ResolveVirtualToPhysical(dstV, dstP))
    {
        r.errors.emplace_back("Failed to resolve physical paths.");
        return r;
    }

    if (!UFileSystem::FileExists(srcP))
    {
        r.errors.emplace_back("Source asset does not exist: " + srcV);
        return r;
    }

    if (!UFileSystem::MoveFile(srcP, dstP))
    {
        r.errors.emplace_back("Failed to rename asset: " + srcP + " -> " + dstP);
        return r;
    }

    // Sync parent folder once (both removed+added happen there)
    FAssetOpResult sync = SyncFolderToRegistry(parent);
    r.affectedVirtualFolders = sync.affectedVirtualFolders;

    if (!sync.bSuccess)
    {
        r.errors.insert(r.errors.end(), sync.errors.begin(), sync.errors.end());
        r.warnings.insert(r.warnings.end(), sync.warnings.begin(), sync.warnings.end());
        r.bSuccess = false;
        return r;
    }

    r.warnings.insert(r.warnings.end(), sync.warnings.begin(), sync.warnings.end());
    r.bSuccess = true;
    return r;
}

FAssetOpResult AssetManager::MoveAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualFolder)
{
    FAssetOpResult r;

    if (!m_PathMounter || !m_Registry)
    {
        r.errors.emplace_back("AssetManager not initialized (Registry/PathMounter is null).");
        return r;
    }

    std::string srcV = NormalizeVirtualPath(sourceVirtualAssetPath);
    std::string dstFolderV = NormalizeVirtualFolder(destVirtualFolder);

    if (!IsWritableVirtualPath(srcV) || !IsWritableVirtualPath(dstFolderV))
    {
        r.errors.emplace_back("Move must stay inside writable mount: " + srcV + " -> " + dstFolderV);
        return r;
    }

    if (!EndsWith(srcV, ".jasset"))
    {
        r.errors.emplace_back("Not an asset file (.jasset): " + srcV);
        return r;
    }

    // Destination folder must exist (professional + explicit)
    bool bFolderExists = false;
    if (!VirtualFolderExistsOnDisk(dstFolderV, bFolderExists))
    {
        r.errors.emplace_back("Failed to resolve destination folder: " + dstFolderV);
        return r;
    }
    if (!bFolderExists)
    {
        r.errors.emplace_back("Destination folder does not exist: " + dstFolderV);
        return r;
    }

    const std::string leaf = GetLeafName(srcV);
    const std::string dstV = JoinVirtual(dstFolderV, leaf);

    bool bDestExists = false;
    if (!VirtualFileExistsOnDisk(dstV, bDestExists))
    {
        r.errors.emplace_back("Failed to resolve destination asset path: " + dstV);
        return r;
    }
    if (bDestExists)
    {
        r.errors.emplace_back("Name collision: destination asset already exists: " + dstV);
        return r;
    }

    std::string srcP, dstP;
    if (!ResolveVirtualToPhysical(srcV, srcP) || !ResolveVirtualToPhysical(dstV, dstP))
    {
        r.errors.emplace_back("Failed to resolve physical paths.");
        return r;
    }

    if (!UFileSystem::MoveFile(srcP, dstP))
    {
        r.errors.emplace_back("Failed to move asset: " + srcP + " -> " + dstP);
        return r;
    }

    // Sync both source parent and destination folder
    const std::string srcParent = NormalizeVirtualFolder(GetParentFolder(srcV));

    FAssetOpResult syncA = SyncFolderToRegistry(srcParent);
    FAssetOpResult syncB = (srcParent == dstFolderV) ? FAssetOpResult{true} : SyncFolderToRegistry(dstFolderV);

    r.affectedVirtualFolders.insert(r.affectedVirtualFolders.end(),
                                   syncA.affectedVirtualFolders.begin(), syncA.affectedVirtualFolders.end());
    r.affectedVirtualFolders.insert(r.affectedVirtualFolders.end(),
                                   syncB.affectedVirtualFolders.begin(), syncB.affectedVirtualFolders.end());

    if (!syncA.bSuccess || !syncB.bSuccess)
    {
        r.errors.emplace_back("Registry sync failed during asset move.");
        r.errors.insert(r.errors.end(), syncA.errors.begin(), syncA.errors.end());
        r.errors.insert(r.errors.end(), syncB.errors.begin(), syncB.errors.end());
        r.warnings.insert(r.warnings.end(), syncA.warnings.begin(), syncA.warnings.end());
        r.warnings.insert(r.warnings.end(), syncB.warnings.begin(), syncB.warnings.end());
        r.bSuccess = false;
        return r;
    }

    r.warnings.insert(r.warnings.end(), syncA.warnings.begin(), syncA.warnings.end());
    r.warnings.insert(r.warnings.end(), syncB.warnings.begin(), syncB.warnings.end());
    r.bSuccess = true;
    return r;
}

FAssetOpResult AssetManager::DuplicateAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualAssetPath)
{
    FAssetOpResult r;

    if (!m_PathMounter || !m_Registry)
    {
        r.errors.emplace_back("AssetManager not initialized (Registry/PathMounter is null).");
        return r;
    }

    std::string srcV = NormalizeVirtualPath(sourceVirtualAssetPath);
    std::string dstV = NormalizeVirtualPath(destVirtualAssetPath);

    if (!IsWritableVirtualPath(dstV))
    {
        r.errors.emplace_back("Destination path is not writable: " + dstV);
        return r;
    }

    if (!EndsWith(srcV, ".jasset") || !EndsWith(dstV, ".jasset"))
    {
        r.errors.emplace_back("DuplicateAsset requires .jasset source and destination.");
        return r;
    }

    // Collision check (destination)
    bool bDestExists = false;
    if (!VirtualFileExistsOnDisk(dstV, bDestExists))
    {
        r.errors.emplace_back("Failed to resolve destination asset path: " + dstV);
        return r;
    }
    if (bDestExists)
    {
        r.errors.emplace_back("Name collision: destination asset already exists: " + dstV);
        return r;
    }

    std::string srcP, dstP;
    if (!ResolveVirtualToPhysical(srcV, srcP) || !ResolveVirtualToPhysical(dstV, dstP))
    {
        r.errors.emplace_back("Failed to resolve physical paths.");
        return r;
    }

    if (!UFileSystem::FileExists(srcP))
    {
        r.errors.emplace_back("Source asset does not exist: " + srcV);
        return r;
    }

    if (!UFileSystem::CopyFile(srcP, dstP))
    {
        r.errors.emplace_back("Failed to copy asset: " + srcP + " -> " + dstP);
        return r;
    }

    const std::string newID = UUUID::GenerateUUID();
    if (!AssetFile::RewriteAssetID(dstP, newID))
    {
        r.errors.emplace_back("Failed to rewrite AssetID in duplicated asset.");
        return r;
    }

    const std::string dstParent = NormalizeVirtualFolder(GetParentFolder(dstV));
    FAssetOpResult sync = SyncFolderToRegistry(dstParent);

    r.affectedVirtualFolders = sync.affectedVirtualFolders;
    if (!sync.bSuccess)
    {
        r.errors.insert(r.errors.end(), sync.errors.begin(), sync.errors.end());
        r.warnings.insert(r.warnings.end(), sync.warnings.begin(), sync.warnings.end());
        r.bSuccess = false;
        return r;
    }

    r.warnings.insert(r.warnings.end(), sync.warnings.begin(), sync.warnings.end());
    r.bSuccess = true;
    return r;
}

// =====================================================================
// Policy / Resolve
// =====================================================================

bool AssetManager::IsWritableVirtualPath(const std::string& virtualPath) const
{
    return virtualPath.rfind("/Project", 0) == 0;
}

bool AssetManager::ResolveVirtualToPhysical(const std::string &virtualPath, std::string &outPhysicalPath) const
{
    if (!m_PathMounter) return false;
    return m_PathMounter->ResolveVirtualToPhysical(virtualPath, outPhysicalPath);
}

// =====================================================================
// Helpers (normalize / join / parse)
// =====================================================================

std::string AssetManager::NormalizeVirtualPath(std::string v)
{
    if (v.size() > 1 && v.back() == '/')
        v.pop_back();
    return v;
}

std::string AssetManager::NormalizeVirtualFolder(std::string v)
{
    if (v.size() > 1 && v.back() == '/')
        v.pop_back();
    return v;
}

// "/Project/Folder/Asset.jasset" -> "/Project/Folder"
std::string AssetManager::GetParentFolder(const std::string& virtualPath)
{
    const std::size_t pos = virtualPath.find_last_of('/');
    if (pos == std::string::npos || pos == 0)
        return virtualPath;
    return virtualPath.substr(0, pos);
}

std::string AssetManager::GetLeafName(const std::string& virtualPath)
{
    const std::size_t pos = virtualPath.find_last_of('/');
    return (pos == std::string::npos) ? virtualPath : virtualPath.substr(pos + 1);
}

std::string AssetManager::JoinVirtual(const std::string& parent, const std::string& leaf)
{
    if (parent.empty()) return leaf;
    if (parent == "/") return "/" + leaf;
    return parent + "/" + leaf;
}

bool AssetManager::EndsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool AssetManager::IsSimpleName(const std::string& name)
{
    if (name.empty()) return false;
    if (name.find('/') != std::string::npos) return false;
    if (name.find('\\') != std::string::npos) return false;
    return true;
}

// =====================================================================
// Collision helpers (disk truth)
// =====================================================================

bool AssetManager::VirtualFolderExistsOnDisk(const std::string& virtualFolder, bool& outExists) const
{
    outExists = false;
    std::string phys;
    if (!ResolveVirtualToPhysical(virtualFolder, phys))
        return false;
    outExists = UFileSystem::DirectoryExists(phys);
    return true;
}

bool AssetManager::VirtualFileExistsOnDisk(const std::string& virtualFile, bool& outExists) const
{
    outExists = false;
    std::string phys;
    if (!ResolveVirtualToPhysical(virtualFile, phys))
        return false;
    outExists = UFileSystem::FileExists(phys);
    return true;
}

// =====================================================================
// Sync (scan disk -> replace registry)
// =====================================================================

FAssetOpResult AssetManager::SyncFolderToRegistry(const std::string& virtualFolder)
{
    FAssetOpResult r;
    const std::string v = NormalizeVirtualFolder(virtualFolder);
    r.affectedVirtualFolders.push_back(v);

    if (!m_Registry || !m_PathMounter)
    {
        r.errors.emplace_back("AssetManager not initialized (Registry/PathMounter is null).");
        return r;
    }

    AssetRegistryScanner scanner;
    FAssetScanResult scan = scanner.ScanFolder(*m_PathMounter, v);

    auto upd = m_Registry->ReplaceFolderContents(v, std::move(scan.records));

    if (!upd.bSuccess)
    {
        r.errors.emplace_back("Registry ReplaceFolderContents failed for: " + v);
        // If your update result contains errors/warnings, append them here.
        r.bSuccess = false;
        return r;
    }

    if (!scan.bSuccess)
        r.warnings.emplace_back("Folder scan completed with warnings/errors: " + v);

    r.bSuccess = true;
    return r;
}

FAssetOpResult AssetManager::SyncRootToRegistry(const std::string& virtualRoot)
{
    FAssetOpResult r;
    const std::string v = NormalizeVirtualFolder(virtualRoot);
    r.affectedVirtualFolders.push_back(v);

    if (!m_Registry || !m_PathMounter)
    {
        r.errors.emplace_back("AssetManager not initialized (Registry/PathMounter is null).");
        return r;
    }

    AssetRegistryScanner scanner;
    FAssetScanResult scan = scanner.ScanRoot(*m_PathMounter, v);

    auto upd = m_Registry->ReplaceFolderContents(v, std::move(scan.records));

    if (!upd.bSuccess)
    {
        r.errors.emplace_back("Registry ReplaceFolderContents failed for: " + v);
        r.bSuccess = false;
        return r;
    }

    if (!scan.bSuccess)
        r.warnings.emplace_back("Root scan completed with warnings/errors: " + v);

    r.bSuccess = true;
    return r;
}