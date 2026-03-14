// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

class VirtualPathMounter;
class AssetRegistrySubsystem;
struct FAssetRecord;

class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;

private:
    AssetRegistrySubsystem* m_Registry = nullptr;
    VirtualPathMounter* m_PathMounter = nullptr;

public:
    bool Initialize(AssetRegistrySubsystem* registry, VirtualPathMounter* pathMounter);
    void Shutdown();

    bool RebuildRegistry();

    [[nodiscard]] const FAssetRecord* FindByAssetID(const std::string& assetID) const;
    [[nodiscard]] const FAssetRecord* FindByVirtualPath(const std::string& virtualPath) const;
    [[nodiscard]] const FAssetRecord* FindByPhysicalPath(const std::string& physicalPath) const;

    [[nodiscard]] const std::vector<FAssetRecord>* GetAllAssets() const;
};