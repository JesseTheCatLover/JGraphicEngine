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
     * @brief Rebuild the entire asset registry by scanning mounted roots.
     */
    bool RebuildRegistry();

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
};
