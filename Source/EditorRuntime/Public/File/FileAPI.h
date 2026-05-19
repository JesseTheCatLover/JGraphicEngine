//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>
#include "Assets/AssetTypes.h"
#include "Assets/EAssetDomain.h"
#include "Rendering/RHandles.h"

struct FAssetImportResult;
struct FAssetImportRequest;
struct FAssetRecord;
class VirtualPathMounter;
class AssetManager;
class ResourceSubsystem;
class EngineContext;

class EditorFileAPI
{
    private:
    EngineContext& m_Context;
    ResourceSubsystem& m_Resource;
    AssetManager& m_AssetManager;
    VirtualPathMounter& m_PathMounter;

public:
    EditorFileAPI(EngineContext& ctx, ResourceSubsystem& resource, VirtualPathMounter& pathMounter, AssetManager& assetManager);

    RTextureHandle LoadEditorTextureFromFile(const char* virtualFilePath, bool bSRGB = true);

    [[nodiscard]] const std::vector<FAssetRecord>* GetAllAssets() const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByPrefix(const std::string& virtualPrefix) const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetUserVisibleAssets() const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByType(EAssetType type) const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByDomain(EAssetDomain domain) const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByVisibility(EAssetVisibility visibility) const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetDependencies(const std::string& assetID) const;
    [[nodiscard]] const FAssetRecord* FindByAssetID(const std::string& assetID) const;
    [[nodiscard]] const FAssetRecord* FindByVirtualPath(const std::string& virtualPath) const;

    bool ImportAsset(const FAssetImportRequest& request, FAssetImportResult& outResult);

    std::vector<FAssetImportResult> ImportAssetsBatch(const std::vector<FAssetImportRequest>& requests);
};
