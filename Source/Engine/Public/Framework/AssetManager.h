// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

struct FAssetImportResult;
struct FAssetImportRequest;
struct FAssetRecord;
class VirtualPathMounter;
class AssetRegistrySubsystem;
class AssetImportSubsystem;

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
    AssetImportSubsystem* m_Importer = nullptr;
    VirtualPathMounter* m_PathMounter = nullptr;

public:
    bool Initialize(AssetRegistrySubsystem* registry,
                    AssetImportSubsystem* importer,
                    VirtualPathMounter* pathMounter);
    void Shutdown();

    bool RebuildRegistry();

    bool ImportAsset(const FAssetImportRequest& request, FAssetImportResult& outResult);

    [[nodiscard]] const FAssetRecord* FindByAssetID(const std::string& assetID) const;
    [[nodiscard]] const FAssetRecord* FindByVirtualPath(const std::string& virtualPath) const;
    [[nodiscard]] const FAssetRecord* FindByPhysicalPath(const std::string& physicalPath) const;

    [[nodiscard]] const std::vector<FAssetRecord>* GetAllAssets() const;
    [[nodiscard]] std::vector<const FAssetRecord*> GetAssetsByPrefix(const std::string& virtualPrefix) const;
};