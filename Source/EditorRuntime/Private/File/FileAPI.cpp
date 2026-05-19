//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "File/FileAPI.h"

#include "Assets/FAssetImportRequest.h"
#include "Assets/FAssetImportResult.h"
#include "Assets/FAssetRecord.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Framework/AssetManager.h"
#include "Resources/ResourceSubsystem.h"
#include "Resources/GpuResources/Texture2DResource.h"

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

    const FAssetRecord* record = m_AssetManager.FindByVirtualPath(virtualPath);

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

        record = m_AssetManager.FindByVirtualPath(virtualPath);
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

const std::vector<FAssetRecord>* EditorFileAPI::GetAllAssets() const
{
    return m_AssetManager.GetAllAssets();
}

std::vector<const FAssetRecord *> EditorFileAPI::GetAssetsByPrefix(const std::string &virtualPrefix) const
{
    return m_AssetManager.GetAssetsByPrefix(virtualPrefix);
}

std::vector<const FAssetRecord*> EditorFileAPI::GetUserVisibleAssets() const
{
    return m_AssetManager.GetUserVisibleAssets();
}

std::vector<const FAssetRecord*> EditorFileAPI::GetAssetsByType(EAssetType type) const
{
    return m_AssetManager.GetAssetsByType(type);
}

std::vector<const FAssetRecord*> EditorFileAPI::GetAssetsByDomain(EAssetDomain domain) const
{
    return m_AssetManager.GetAssetsByDomain(domain);
}

std::vector<const FAssetRecord*> EditorFileAPI::GetAssetsByVisibility(EAssetVisibility visibility) const
{
    return m_AssetManager.GetAssetsByVisibility(visibility);
}

std::vector<const FAssetRecord*> EditorFileAPI::GetDependencies(const std::string& assetID) const
{
    return m_AssetManager.GetDependencies(assetID);
}

const FAssetRecord* EditorFileAPI::FindByAssetID(const std::string& assetID) const
{
    return m_AssetManager.FindByAssetID(assetID);
}

const FAssetRecord* EditorFileAPI::FindByVirtualPath(const std::string& virtualPath) const
{
    return m_AssetManager.FindByVirtualPath(virtualPath);
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
