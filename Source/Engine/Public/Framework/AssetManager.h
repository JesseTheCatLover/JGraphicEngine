// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "Assets/FAssetRecord.h"
#include "Assets/FAssetImportRequest.h"
#include "Assets/FAssetOpResult.h"
#include "Assets/FVirtualDirEntry.h"

struct FAssetImportResult;
class AssetRegistrySubsystem;
class AssetImportSubsystem;
class VirtualPathMounter;

/**
 * @class AssetManager
 *
 * @brief High-level facade, responsible for coordinating the engine's asset system.
 *
 * The AssetManager provides a unified interface for interacting with assets and folders
 * within the engine.
 *
 * Ownership note (folder ops):
 *  - AssetManager owns physical disk operations for asset storage (folders/files)
 *  - Uses AssetRegistryScanner to read from disk
 *  - AssetRegistrySubsystem remains a pure in-memory index
 *  - EditorFileAPI should be a thin facade that calls AssetManager
 **/
class AssetManager
{
private:
    AssetRegistrySubsystem* m_Registry = nullptr;
    AssetImportSubsystem* m_Importer = nullptr;
    VirtualPathMounter* m_PathMounter = nullptr;

    bool bInitialSynced = false;

public:
    AssetManager() = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;

private:
    friend class JEngine;
    /**
     * @brief Startup-time: scan mounted roots on disk and populate the in-memory registry.
     *
     * Responsibilities:
     * - Enumerate mounted roots (Engine/Project/Plugins/ecs...).
     * - Scan .jasset files under those roots (via AssetRegistryScanner).
     * - Bulk replace registry contents per-root (ReplaceFolderContents).
     *
     * Notes:
     * - Designed for engine/editor boot.
     * - After this, normal mutations keep registry in sync via AssetManager ops/watchers.
     */
    [[nodiscard]] FAssetOpResult InitialSyncRegistryFromDisk();

public:

    /**
     * @brief Initializes the AssetManager and binds subsystems to self.
     */
    bool Initialize(
        AssetRegistrySubsystem* registry,
        AssetImportSubsystem* importer,
        VirtualPathMounter* pathMounter);

    /**
     * @brief Shuts down the asset manager.
     */
    void Shutdown();

    /**
     * @brief Import an asset into the project.
     *
     * The importer determines the correct importer plugin, based on the
     * source file and writes the resulting ".jasset" file.
     */
    bool ImportAsset(
        const FAssetImportRequest& request,
        FAssetImportResult& outResult);

    /**
     * @brief Find an asset by its unique AssetID.
     */
    [[nodiscard]]
    const FAssetRecord* FindAssetByAssetID(const std::string& assetID) const;

    /**
     * @brief Find an asset by its virtual path.
     * Example: /Project/Textures/Wood.jasset
     */
    [[nodiscard]]
    const FAssetRecord* FindAssetByVirtualPath(const std::string& virtualPath) const;

    /**
     * @brief Returns assets under a virtual path prefix.
     * Example: /Project/Textures
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> FindAllAssetsByVirtualPathPrefix(const std::string& virtualPathPrefix) const;

    /**
     * @brief Find an asset by its physical disk path.
     */
    [[nodiscard]]
    const FAssetRecord* FindAssetByPhysicalPath(const std::string& physicalPath) const;

    /**
     * @brief Returns every asset registered in the system.
     */
    [[nodiscard]]
    const std::vector<FAssetRecord>* GetAllAssets() const;

    /**
     * @brief Returns all assets visible to the user (Filters out EnginePrivate).
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetAllUserVisibleAssets() const;

    /**
     * @brief Returns assets of a specific asset type.
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetAllAssetsByType(EAssetType type) const;

    /**
     * @brief Returns assets belonging to a specific domain (Engine or Project).
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetAllAssetsByDomain(EAssetDomain domain) const;

    /**
     * @brief Returns assets with a specific visibility classification.
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetAllAssetsByVisibility(EAssetVisibility visibility) const;

    /**
     * @brief Returns dependencies for a specific asset.
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetAllDependenciesForAsset(const std::string& assetID) const;

    // ---------------------------------------------------------------------
    // Physical folder operations
    // Strategy: perform disk operation first, then rebuild registry from disk.
    // ---------------------------------------------------------------------

    /**
     * @brief List child folders under a virtual folder.
     * Physical query, returned as virtual paths.
     */
    [[nodiscard]]
    std::vector<std::string> ListFolders(const std::string& parentVirtualFolder, bool bRecursive) const;

    /**
     * @brief List immediate children (folders + assets) of a virtual folder.
     * Non-recursive. Returned paths are normalized virtual paths.
     */
    [[nodiscard]]
    std::vector<FVirtualDirEntry> ListDirectory(const std::string& parentVirtualFolder) const;

    /**
     * @brief Create a folder on disk for a given virtual folder path.
     */
    FAssetOpResult CreateFolder(const std::string& folderVirtualPath);

    /**
     * @brief Delete a folder (virtual path) on disk.
     */
    FAssetOpResult DeleteFolder(const std::string& folderVirtualPath, bool bRecursive);

    /**
     * @brief Rename a folder (virtual path) on disk.
     */
    FAssetOpResult RenameFolder(const std::string& oldVirtualPath, const std::string& newVirtualPath);

    /**
     * @brief Move a folder (virtual path) on disk.
     */
    FAssetOpResult MoveFolder(const std::string& sourceVirtualPath, const std::string& destVirtualPath);

    // ---------------------------------------------------------------------
    // Physical Asset (File) operations
    // Strategy: perform disk op on the .jasset file, then sync affected folders.
    // ---------------------------------------------------------------------

    /**
     * @brief Deletes an asset file from disk and removes it from the registry.
     */
    FAssetOpResult DeleteAsset(const std::string& virtualAssetPath);

    /**
     * @brief Renames an asset file on disk. The AssetID remains unchanged.
     */
    FAssetOpResult RenameAsset(const std::string& virtualAssetPath, const std::string& newName);

    /**
     * @brief Moves an asset file to a new folder. The AssetID remains unchanged.
     */
    FAssetOpResult MoveAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualFolder);

    /**
     * @brief Duplicates an asset file.
     */
    FAssetOpResult DuplicateAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualAssetPath);

    /**
     * @brief Saves a newly authored in-engine asset (e.g., a Material or Schematic created in the editor).
     * Unlike ImportAsset which reads external files (PNG/FBX), this serializes engine data directly.
     */
    // FAssetOpResult SaveNewAsset(const std::string& destVirtualAssetPath, ... /* payload/data */);

private:
    // -------------------------
    // Policy / Resolve
    // -------------------------

    // Policy: only allow physical mutations within writable mounts (typically /Project).
    [[nodiscard]] bool IsWritableVirtualPath(const std::string& virtualPath) const;
    [[nodiscard]] bool ResolveVirtualToPhysical(const std::string& virtualPath, std::string& outPhysicalPath) const;

    // -------------------------
    // Validation / composition
    // -------------------------
    static std::string NormalizeVirtualPath(std::string v);
    static std::string NormalizeVirtualFolder(std::string v);
    static std::string GetParentFolder(const std::string& virtualPath);
    static std::string GetLeafName(const std::string& virtualPath);
    static std::string JoinVirtual(const std::string& parent, const std::string& leaf);
    static bool EndsWith(const std::string& s, const std::string& suffix);
    static bool IsSimpleName(const std::string& name);

    // -------------------------
    // Collision checks (one place)
    // -------------------------
    bool VirtualFolderExistsOnDisk(const std::string& virtualFolder, bool& outExists) const;
    bool VirtualFileExistsOnDisk(const std::string& virtualFile, bool& outExists) const;

    // -------------------------
    // Sync (scan disk -> replace registry)
    // -------------------------
    FAssetOpResult SyncFolderToRegistry(const std::string& virtualFolder);
    FAssetOpResult SyncRootToRegistry(const std::string& virtualRoot);
};
