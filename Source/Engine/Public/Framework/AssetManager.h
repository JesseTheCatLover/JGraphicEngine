// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "Assets/FAssetRecord.h"
#include "Assets/FAssetImportRequest.h"

struct FAssetImportResult;
class AssetRegistrySubsystem;
class AssetImportSubsystem;
class VirtualPathMounter;

/**
 * @class AssetManager
 *
 * @brief High‑level facade, responsible for coordinating the engine's asset system.
 *
 * The AssetManager provides a unified interface for interacting with assets
 * within the engine.
 *
 * Ownership note (folder ops):
 *  - AssetManager owns physical disk operations for asset storage (folders/files)
 *  - AssetRegistrySubsystem remains an in-memory index that can be rebuilt from disk
 *  - EditorFileAPI should be a thin facade that calls AssetManager
 **/
class AssetManager
{
private:
    AssetRegistrySubsystem* m_Registry = nullptr;
    AssetImportSubsystem* m_Importer = nullptr;
    VirtualPathMounter* m_PathMounter = nullptr;

public:
    AssetManager() = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

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
    const FAssetRecord* FindByAssetID(const std::string& assetID) const;

    /**
     * @brief Find an asset by its virtual path.
     *
     * Example:
     * /Project/Textures/Wood.jasset
     */
    [[nodiscard]]
    const FAssetRecord* FindByVirtualPath(const std::string& virtualPath) const;

    /**
     * @brief Find an asset by its physical disk path.
     */
    [[nodiscard]]
    const FAssetRecord* FindByPhysicalPath(const std::string& physicalPath) const;

    /**
     * @brief Returns every asset registered in the system.
     */
    [[nodiscard]]
    const std::vector<FAssetRecord>* GetAllAssets() const;

    /**
     * @brief Returns assets under a virtual path prefix.
     *
     * Example:
     * /Project/Textures
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetAssetsByPrefix(
        const std::string& virtualPrefix) const;

    /**
     * @brief Returns all assets visible to the user.
     *
     * Filters out EnginePrivate assets.
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetUserVisibleAssets() const;

    /**
     * @brief Returns assets of a specific asset type.
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetAssetsByType(
        EAssetType type) const;

    /**
     * @brief Returns assets belonging to a specific domain.
     *
     * Domains:
     *  - Engine
     *  - Project
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetAssetsByDomain(
        EAssetDomain domain) const;

    /**
     * @brief Returns assets with a specific visibility classification.
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetAssetsByVisibility(
        EAssetVisibility visibility) const;

    /**
     * @brief Returns dependencies for a specific asset.
     */
    [[nodiscard]]
    std::vector<const FAssetRecord*> GetDependencies(
        const std::string& assetID) const;

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
     * @brief Create a folder on disk for a given virtual folder path.
     *
     * Policy: only allowed in writable mounts.
     * No registry rebuild is needed because empty folders are not assets.
     */
    bool CreateFolder(const std::string& folderVirtualPath);

    /**
     * @brief Rename a folder (virtual path) on disk.
     *
     * If successful, triggers folder-scoped registry rebuilds for both
     * the old and new virtual paths.
     */
    bool DeleteFolder(const std::string& folderVirtualPath, bool bRecursive);

    /**
     * @brief Rename a folder (virtual path) on disk.
     *
     * If successful, triggers folder-scoped registry rebuilds for both
     * the old and new virtual paths.
     */
    bool RenameFolder(const std::string& oldVirtualPath, const std::string& newVirtualPath);

    /**
     * @brief Move a folder (virtual path) on disk.
     */
    bool MoveFolder(const std::string& sourceVirtualPath, const std::string& destVirtualPath);

private:

    // Internal Helpers:

    /**
     * @brief Delete all assets registered under a given virtual folder.
     * Registry-only; physical files are not touched here.
     */
    bool DeleteAssetsInFolder(const std::string& folderVirtualPath);

    /**
     * @brief Move all assets in one virtual folder to another.
     * Updates virtualPath in the registry. Physical files are not touched here.
     */
    bool MoveAssetsInFolder(const std::string& sourceFolderVirtualPath,
                            const std::string& destFolderVirtualPath);

    // Policy: only allow physical mutations within writable mounts (typically /Project).
    [[nodiscard]] bool IsWritableVirtualPath(const std::string& virtualPath) const;

    // Helper: resolve virtual->physical using m_PathMounter.
    [[nodiscard]] bool ResolveVirtualToPhysical(const std::string& virtualPath, std::string& outPhysicalPath) const;
};
