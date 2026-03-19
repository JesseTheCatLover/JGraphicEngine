// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "Assets/FAssetRecord.h"

class VirtualPathMounter;

class AssetRegistrySubsystem // TODO: Implement index map lookup for future + startup caching and cooks
{
public:
    AssetRegistrySubsystem() = default;
    ~AssetRegistrySubsystem() = default;

    AssetRegistrySubsystem(const AssetRegistrySubsystem&) = delete;
    AssetRegistrySubsystem& operator=(const AssetRegistrySubsystem&) = delete;
    AssetRegistrySubsystem(AssetRegistrySubsystem&&) = delete;
    AssetRegistrySubsystem& operator=(AssetRegistrySubsystem&&) = delete;

public:
    void Clear();

    /**
     * @brief Scan all mounted roots and rebuild the registry from .jasset headers.
     */
    bool Rebuild(const VirtualPathMounter& mounter);

    /**
     * @brief Scan a single mounted virtual root like /Engine or /Project.
     */
    bool ScanMount(const VirtualPathMounter& mounter, const std::string& virtualRoot);

public:
    [[nodiscard]] const FAssetRecord* FindByAssetID(const std::string& assetID) const;
    [[nodiscard]] const FAssetRecord* FindByVirtualPath(const std::string& virtualPath) const;
    [[nodiscard]] const FAssetRecord* FindByPhysicalPath(const std::string& physicalPath) const;

public:
    [[nodiscard]] const std::vector<FAssetRecord>& GetAllAssets() const { return m_Assets; }

    /**
     * @brief Returns assets visible to the user (filters EnginePrivate).
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetUserVisibleAssets() const;

    /**
     * @brief Returns all assets under a virtual path prefix.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByPrefix(const std::string& virtualPrefix) const;

    /**
     * @brief Returns assets of a specific type.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByType(EAssetType type) const;

    /**
     * @brief Returns assets belonging to a specific domain.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByDomain(EAssetDomain domain) const;

    /**
     * @brief Returns assets with a specific visibility.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByVisibility(EAssetVisibility visibility) const;

    /**
     * @brief Returns dependency asset records.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetDependencies(const std::string& assetID) const;

private:
    bool RegisterAsset(FAssetRecord record);

    void DetermineDomainAndVisibility(
        FAssetRecord& record,
        const std::string& virtualRoot) const;

private:
    std::vector<FAssetRecord> m_Assets;

    std::unordered_map<std::string, size_t> m_ByAssetID;
    std::unordered_map<std::string, size_t> m_ByVirtualPath;
    std::unordered_map<std::string, size_t> m_ByPhysicalPath;
};
