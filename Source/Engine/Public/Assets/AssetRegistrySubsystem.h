// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "FAssetRecord.h"

class VirtualPathMounter;

class AssetRegistrySubsystem
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

    [[nodiscard]] const FAssetRecord* FindByAssetID(const std::string& assetID) const;
    [[nodiscard]] const FAssetRecord* FindByVirtualPath(const std::string& virtualPath) const;
    [[nodiscard]] const FAssetRecord* FindByPhysicalPath(const std::string& physicalPath) const;

    [[nodiscard]] const std::vector<FAssetRecord>& GetAllAssets() const { return m_Assets; }

private:
    bool RegisterAsset(FAssetRecord record);

private:
    std::vector<FAssetRecord> m_Assets;

    std::unordered_map<std::string, size_t> m_ByAssetID;
    std::unordered_map<std::string, size_t> m_ByVirtualPath;
    std::unordered_map<std::string, size_t> m_ByPhysicalPath;
};