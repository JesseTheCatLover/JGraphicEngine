//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "File/FileAPI.h"

#include "Assets/FAssetImportRequest.h"
#include "Assets/FAssetImportResult.h"
#include "Assets/FAssetRecord.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Framework/AssetManager.h"
#include "Resources/ResourceSubsystem.h"
#include "Resources/GpuResources/Texture2DResource.h"
#include "Utilities/UPath.h"

EditorFileAPI::EditorFileAPI(EngineContext &ctx, ResourceSubsystem &resource, VirtualPathMounter& pathMounter,
                             AssetManager& assetManager):
m_Context(ctx),
m_Resource(resource),
m_PathMounter(pathMounter),
m_AssetManager(assetManager)
{
}

RTextureHandle EditorFileAPI::LoadEditorTextureFromFile(const char* virtualFilePath, bool bSRGB)
{
    if (!virtualFilePath || !virtualFilePath[0])
        return {};

    std::string virtualPath = virtualFilePath;

    const FAssetRecord* record = m_AssetManager.FindAssetByVirtualPath(virtualPath);

    if (!record)
    {
        std::string absolutePath;
        if (!m_PathMounter.ResolveVirtualToPhysical(virtualPath, absolutePath))
        {
            std::cerr << "[EditorFileAPI]: Failed to resolve path: " << virtualPath << "\n";
            return {};
        }

        FAssetImportRequest request;
        request.sourceFilePath = absolutePath;
        request.destinationVirtualFolder = virtualPath;

        FAssetImportResult result;
        if (!m_AssetManager.ImportAsset(request, result))
        {
            std::cerr << "[EditorFileAPI]: Import failed: " << absolutePath << "\n";
            return {};
        }

        record = m_AssetManager.FindAssetByVirtualPath(virtualPath);
        if (!record)
            return {};
    }

    const std::string key = "EditorTex:" + record->assetID;

    Texture2DResource::FDesc desc;
    desc.assetID = record->assetID;
    desc.bSRGB = bSRGB;

    auto res = m_Resource.Load<Texture2DResource>(key.c_str(), desc);

    return res ? res->GetTexture() : RTextureHandle{};
}

const FAssetRecord* EditorFileAPI::FindAssetByAssetID(const std::string& assetID) const
{
    return m_AssetManager.FindAssetByAssetID(assetID);
}

const FAssetRecord* EditorFileAPI::FindAssetByVirtualPath(const std::string& virtualPath) const
{
    return m_AssetManager.FindAssetByVirtualPath(virtualPath);
}

std::vector<const FAssetRecord*> EditorFileAPI::FindAllAssetsByVirtualPathPrefix(const std::string& virtualPathPrefix) const
{
    return m_AssetManager.FindAllAssetsByVirtualPathPrefix(virtualPathPrefix);
}

const FAssetRecord* EditorFileAPI::FindAssetByPhysicalPath(const std::string& physicalPath) const
{
    return m_AssetManager.FindAssetByPhysicalPath(physicalPath);
}

const std::vector<FAssetRecord>* EditorFileAPI::GetAllAssets() const
{
    return m_AssetManager.GetAllAssets();
}

std::vector<const FAssetRecord*> EditorFileAPI::GetAllUserVisibleAssets() const
{
    return m_AssetManager.GetAllUserVisibleAssets();
}

std::vector<const FAssetRecord*> EditorFileAPI::GetAllAssetsByType(EAssetType type) const
{
    return m_AssetManager.GetAllAssetsByType(type);
}

std::vector<const FAssetRecord*> EditorFileAPI::GetAllAssetsByDomain(EAssetDomain domain) const
{
    return m_AssetManager.GetAllAssetsByDomain(domain);
}

std::vector<const FAssetRecord*> EditorFileAPI::GetAllAssetsByVisibility(EAssetVisibility visibility) const
{
    return m_AssetManager.GetAllAssetsByVisibility(visibility);
}

std::vector<const FAssetRecord*> EditorFileAPI::GetAllDependenciesForAsset(const std::string& assetID) const
{
    return m_AssetManager.GetAllDependenciesForAsset(assetID);
}

bool EditorFileAPI::ImportAsset(const FAssetImportRequest &request, FAssetImportResult &outResult)
{
    return m_AssetManager.ImportAsset(request, outResult);
}

std::vector<FAssetImportResult> EditorFileAPI::ImportAssetsBatch(const std::vector<FAssetImportRequest> &requests)
{
    std::vector<FAssetImportResult> results;

    for (const auto& req : requests)
    {
        FAssetImportResult result{};
        m_AssetManager.ImportAsset(req, result);
        results.push_back(result);
    }

    return results;
}

std::vector<std::string> EditorFileAPI::ListFolders(const std::string& parentVirtualFolder, bool bRecursive) const
{
    return m_AssetManager.ListFolders(parentVirtualFolder, bRecursive);
}

std::vector<FVirtualDirEntry> EditorFileAPI::ListDirectory(const std::string& parentVirtualFolder) const
{
    return m_AssetManager.ListDirectory(parentVirtualFolder);
}

bool EditorFileAPI::HasAnyChildFolder(const std::string &parentVirtualFolder) const
{
    const std::string parentV = UPath::Normalize(parentVirtualFolder);

    std::vector<std::string> folders = ListFolders(parentV, /*bRecursive=*/false);
    return !folders.empty();
}

FAssetOpResult EditorFileAPI::CreateFolder(const std::string& folderVirtualPath)
{
    return m_AssetManager.CreateFolder(folderVirtualPath);
}

FAssetOpResult EditorFileAPI::DeleteFolder(const std::string& folderVirtualPath, bool bRecursive)
{
    return m_AssetManager.DeleteFolder(folderVirtualPath, bRecursive);
}

FAssetOpResult EditorFileAPI::RenameFolder(const std::string& oldVirtualPath, const std::string& newVirtualPath)
{
    return m_AssetManager.RenameFolder(oldVirtualPath, newVirtualPath);
}

FAssetOpResult EditorFileAPI::MoveFolder(const std::string& sourceVirtualPath, const std::string& destVirtualPath)
{
    return m_AssetManager.MoveFolder(sourceVirtualPath, destVirtualPath);
}

FAssetOpResult EditorFileAPI::DeleteAsset(const std::string& virtualAssetPath)
{
    return m_AssetManager.DeleteAsset(virtualAssetPath);
}

FAssetOpResult EditorFileAPI::RenameAsset(const std::string& virtualAssetPath, const std::string& newName)
{
    return m_AssetManager.RenameAsset(virtualAssetPath, newName);
}

FAssetOpResult EditorFileAPI::MoveAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualFolder)
{
    return m_AssetManager.MoveAsset(sourceVirtualAssetPath, destVirtualFolder);
}

FAssetOpResult EditorFileAPI::DuplicateAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualAssetPath)
{
    return m_AssetManager.DuplicateAsset(sourceVirtualAssetPath, destVirtualAssetPath);
}