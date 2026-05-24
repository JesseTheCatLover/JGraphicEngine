// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Framework/AssetManager.h"

#include <algorithm>
#include <iostream>

#include "Assets/AssetImportSubsystem.h"
#include "Assets/AssetRegistrySubsystem.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Utilities/UFileSystem.h"

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
    std::vector<std::string> foldersToRebuild;
    foldersToRebuild.reserve(outResult.createdAssets.size());

    for (const FImportedAssetInfo& info : outResult.createdAssets)
    {
        if (info.virtualPath.empty())
            continue;

        const std::string folderVirtualPath = GetParentFolder(info.virtualPath);

        // De-duplicate small list with linear search. If this grows, use a set.
        auto it = std::find(foldersToRebuild.begin(), foldersToRebuild.end(), folderVirtualPath);
        if (it == foldersToRebuild.end())
            foldersToRebuild.push_back(folderVirtualPath);
    }

    if (!foldersToRebuild.empty())
    {
        for (const std::string& folder : foldersToRebuild)
        {
            if (!m_Registry->RebuildFolder(*m_PathMounter, folder))
            {
                outResult.warnings.emplace_back(
                    "Asset imported, but folder-scoped registry rebuild reported errors for: " + folder);
            }
        }
    }
    else
    {
        // Fallback: if we somehow have no virtual paths, at least refresh /Project.
        if (!m_Registry->RebuildRoot(*m_PathMounter, "/Project"))
        {
            outResult.warnings.emplace_back(
                "Asset imported, but /Project registry rebuild reported errors.");
        }
    }

    if (!outResult.errors.empty())
    {
        std::cerr << "[AssetManager]: Error(s) while loading "
                  << request.sourceFilePath << ":\n";

        for (const auto& error : outResult.errors)
            std::cerr << error << "\n";

        outResult.bSuccess = false;
        return false;
    }

    if (!outResult.warnings.empty())
    {
        std::cout << "[AssetManager]: Warning(s) while loading "
                  << request.sourceFilePath << ":\n";

        for (const auto& warning : outResult.warnings)
            std::cout << warning << "\n";
    }

    outResult.bSuccess = true;
    return true;
}

const FAssetRecord* AssetManager::FindByAssetID(const std::string& assetID) const
{
    return m_Registry ? m_Registry->FindByAssetID(assetID) : nullptr;
}

const FAssetRecord* AssetManager::FindByVirtualPath(const std::string& virtualPath) const
{
    return m_Registry ? m_Registry->FindByVirtualPath(virtualPath) : nullptr;
}

const FAssetRecord* AssetManager::FindByPhysicalPath(const std::string& physicalPath) const
{
    return m_Registry ? m_Registry->FindByPhysicalPath(physicalPath) : nullptr;
}

const std::vector<FAssetRecord>* AssetManager::GetAllAssets() const
{
    return m_Registry ? &m_Registry->GetAllAssets() : nullptr;
}

std::vector<const FAssetRecord*> AssetManager::GetAssetsByPrefix(const std::string& virtualPrefix) const
{
    if (!m_Registry)
    {
        std::cerr << "[AssetManager]: Registry is null\n";
        return {};
    }

    return m_Registry->GetAssetsByPrefix(virtualPrefix);
}

std::vector<const FAssetRecord*> AssetManager::GetUserVisibleAssets() const
{
    if (!m_Registry)
    {
        std::cerr << "[AssetManager]: Registry is null\n";
        return {};
    }

    return m_Registry->GetUserVisibleAssets();
}

std::vector<const FAssetRecord*> AssetManager::GetAssetsByType(EAssetType type) const
{
    if (!m_Registry)
    {
        std::cerr << "[AssetManager]: Registry is null\n";
        return {};
    }

    return m_Registry->GetAssetsByType(type);
}

std::vector<const FAssetRecord*> AssetManager::GetAssetsByDomain(EAssetDomain domain) const
{
    if (!m_Registry)
    {
        std::cerr << "[AssetManager]: Registry is null\n";
        return {};
    }

    return m_Registry->GetAssetsByDomain(domain);
}

std::vector<const FAssetRecord*> AssetManager::GetAssetsByVisibility(EAssetVisibility visibility) const
{
    if (!m_Registry)
    {
        std::cerr << "[AssetManager]: Registry is null\n";
        return {};
    }

    return m_Registry->GetAssetsByVisibility(visibility);
}

std::vector<const FAssetRecord*> AssetManager::GetDependencies(const std::string& assetID) const
{
    if (!m_Registry)
    {
        std::cerr << "[AssetManager]: Registry is null\n";
        return {};
    }

    return m_Registry->GetDependencies(assetID);
}

std::vector<std::string> AssetManager::ListFolders(const std::string& parentVirtualFolder,
                                                   bool bRecursive) const
{
    std::vector<std::string> result;

    if (!m_PathMounter)
    {
        std::cerr << "[AssetManager::ListFolders]: Path mounter is null\n";
        return result;
    }

    std::string physicalParent;
    if (!ResolveVirtualToPhysical(parentVirtualFolder, physicalParent))
    {
        std::cerr << "[AssetManager::ListFolders]: Failed to resolve virtual path: "
                  << parentVirtualFolder << "\n";
        return result;
    }

    std::vector<std::string> physicalDirs = UFileSystem::ListDirectories(physicalParent, bRecursive);

    result.reserve(physicalDirs.size());
    for (const std::string& physDir : physicalDirs)
    {
        std::string virtualDir;
        if (!m_PathMounter->ResolvePhysicalToVirtual(physDir, virtualDir))
            continue;

        result.push_back(virtualDir);
    }

    return result;
}

bool AssetManager::CreateFolder(const std::string& folderVirtualPath)
{
    if (!m_PathMounter)
        return false;

    if (!IsWritableVirtualPath(folderVirtualPath))
    {
        std::cerr << "[AssetManager::CreateFolder]: Path is not writable: "
                  << folderVirtualPath << "\n";
        return false;
    }

    std::string physicalPath;
    if (!ResolveVirtualToPhysical(folderVirtualPath, physicalPath))
    {
        std::cerr << "[AssetManager::CreateFolder]: Failed to resolve virtual path: "
                  << folderVirtualPath << "\n";
        return false;
    }

    if (!UFileSystem::CreateDirectory(physicalPath))
    {
        std::cerr << "[AssetManager::CreateFolder]: Failed to create directory: "
                  << physicalPath << "\n";
        return false;
    }

    // No rebuild needed; folders alone are not assets.
    return true;
}

bool AssetManager::DeleteFolder(const std::string& folderVirtualPath, bool bRecursive)
{
    if (!m_Registry || !m_PathMounter)
        return false;

    if (!IsWritableVirtualPath(folderVirtualPath))
    {
        std::cerr << "[AssetManager::DeleteFolder]: Path is not writable: "
                  << folderVirtualPath << "\n";
        return false;
    }

    std::string physicalPath;
    if (!ResolveVirtualToPhysical(folderVirtualPath, physicalPath))
    {
        std::cerr << "[AssetManager::DeleteFolder]: Failed to resolve virtual path: "
                  << folderVirtualPath << "\n";
        return false;
    }

    if (!UFileSystem::DeleteDirectory(physicalPath, bRecursive))
    {
        std::cerr << "[AssetManager::DeleteFolder]: Failed to delete directory: "
                  << physicalPath << "\n";
        return false;
    }

    // Disk is truth; rebuild only this folder in the registry.
    if (!m_Registry->RebuildFolder(*m_PathMounter, folderVirtualPath))
    {
        std::cerr << "[AssetManager::DeleteFolder]: Registry rebuild for folder failed: "
                  << folderVirtualPath << "\n";
        return false;
    }

    return true;
}

bool AssetManager::RenameFolder(const std::string& oldVirtualPath,
                                const std::string& newVirtualPath)
{
    if (!m_Registry || !m_PathMounter)
        return false;

    if (!IsWritableVirtualPath(oldVirtualPath) || !IsWritableVirtualPath(newVirtualPath))
    {
        std::cerr << "[AssetManager::RenameFolder]: Rename must stay inside writable mount: "
                  << oldVirtualPath << " -> " << newVirtualPath << "\n";
        return false;
    }

    std::string oldPhysical;
    std::string newPhysical;

    if (!ResolveVirtualToPhysical(oldVirtualPath, oldPhysical))
    {
        std::cerr << "[AssetManager::RenameFolder]: Failed to resolve old virtual path: "
                  << oldVirtualPath << "\n";
        return false;
    }

    if (!ResolveVirtualToPhysical(newVirtualPath, newPhysical))
    {
        std::cerr << "[AssetManager::RenameFolder]: Failed to resolve new virtual path: "
                  << newVirtualPath << "\n";
        return false;
    }

    // Perform physical move first; then rebuild registry so registry matches disk.
    if (!UFileSystem::MoveDirectory(oldPhysical, newPhysical))
    {
        std::cerr << "[AssetManager::RenameFolder]: Failed to move directory: "
                  << oldPhysical << " -> " << newPhysical << "\n";
        return false;
    }

    // Rebuild both old and new ranges; cheap and keeps things accurate.
    if (!m_Registry->RebuildFolder(*m_PathMounter, oldVirtualPath))
    {
        std::cerr << "[AssetManager::RenameFolder]: Registry rebuild for old folder failed: "
                  << oldVirtualPath << "\n";
        return false;
    }

    if (!m_Registry->RebuildFolder(*m_PathMounter, newVirtualPath))
    {
        std::cerr << "[AssetManager::RenameFolder]: Registry rebuild for new folder failed: "
                  << newVirtualPath << "\n";
        return false;
    }

    return true;
}

bool AssetManager::MoveFolder(const std::string& sourceVirtualPath,
                              const std::string& destVirtualPath)
{
    return RenameFolder(sourceVirtualPath, destVirtualPath);
}

bool AssetManager::DeleteAssetsInFolder(const std::string& folderVirtualPath)
{
    if (!m_Registry)
    {
        std::cerr << "[AssetManager]: Registry is null in DeleteAssetsInFolder\n";
        return false;
    }

    return m_Registry->DeleteAssetsInFolder(folderVirtualPath);
}

bool AssetManager::MoveAssetsInFolder(const std::string& sourceFolderVirtualPath, const std::string& destFolderVirtualPath)
{
    if (!m_Registry)
    {
        std::cerr << "[AssetManager]: Registry is null in MoveAssetsInFolder\n";
        return false;
    }

    return m_Registry->MoveAssetsInFolder(sourceFolderVirtualPath, destFolderVirtualPath);
}

bool AssetManager::IsWritableVirtualPath(const std::string& virtualPath) const
{
    // Policy v1: only /Project is writable; /Engine is read-only.
    // We can later formalize this via mount metadata.
    return virtualPath.rfind("/Project", 0) == 0;
}

bool AssetManager::ResolveVirtualToPhysical(const std::string &virtualPath, std::string &outPhysicalPath) const
{
    if (!m_PathMounter)
        return false;

    return m_PathMounter->ResolveVirtualToPhysical(virtualPath, outPhysicalPath);
}
