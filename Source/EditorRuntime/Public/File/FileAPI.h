//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>
#include "Assets/AssetTypes.h"
#include "Assets/EAssetDomain.h"
#include "Assets/FAssetOpResult.h"
#include "Assets/FVirtualDirEntry.h"
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

    [[nodiscard]] const FAssetRecord* FindAssetByAssetID(const std::string& assetID) const;
    [[nodiscard]] const FAssetRecord* FindAssetByVirtualPath(const std::string& virtualPath) const;
    [[nodiscard]] std::vector<const FAssetRecord*> FindAllAssetsByVirtualPathPrefix(const std::string& virtualPathPrefix) const;
    [[nodiscard]] const FAssetRecord* FindAssetByPhysicalPath(const std::string& physicalPath) const;

    [[nodiscard]] const std::vector<FAssetRecord>* GetAllAssets() const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllUserVisibleAssets() const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllAssetsByType(EAssetType type) const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllAssetsByDomain(EAssetDomain domain) const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllAssetsByVisibility(EAssetVisibility visibility) const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllDependenciesForAsset(const std::string& assetID) const;

    bool ImportAsset(const FAssetImportRequest& request, FAssetImportResult& outResult);

    std::vector<FAssetImportResult> ImportAssetsBatch(const std::vector<FAssetImportRequest>& requests);

    [[nodiscard]] std::vector<std::string> ListFolders(const std::string& parentVirtualFolder,
                                                       bool bRecursive = false) const;

    [[nodiscard]] std::vector<FVirtualDirEntry> ListDirectory(const std::string& parentVirtualFolder) const;

    [[nodiscard]] FAssetOpResult CreateFolder(const std::string& folderVirtualPath);
    [[nodiscard]] FAssetOpResult DeleteFolder(const std::string& folderVirtualPath, bool bRecursive = true);
    [[nodiscard]] FAssetOpResult RenameFolder(const std::string& oldVirtualPath, const std::string& newVirtualPath);
    [[nodiscard]] FAssetOpResult MoveFolder(const std::string& sourceVirtualPath, const std::string& destVirtualPath);

    [[nodiscard]] FAssetOpResult DeleteAsset(const std::string& virtualAssetPath);
    [[nodiscard]] FAssetOpResult RenameAsset(const std::string& virtualAssetPath, const std::string& newName);
    [[nodiscard]] FAssetOpResult MoveAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualFolder);
    [[nodiscard]] FAssetOpResult DuplicateAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualAssetPath);
};
