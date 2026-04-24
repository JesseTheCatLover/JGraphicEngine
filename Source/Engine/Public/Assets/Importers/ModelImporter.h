//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <string>

#include "AssetImporterBase.h"
#include "IAssetImporter.h"

class AssetImportSubsystem;
class VirtualPathMounter;
struct FAssetImportRequest;
struct FAssetImportResult;

class ModelImporter : public AssetImporterBase
{
private:
    // Cache: absolute texture path -> texture AssetID
    mutable std::unordered_map<std::string, std::string> m_TextureCache;

    std::string ImportTextureIfNeeded(
    const std::string& texturePath,
    const std::string& modelSourceDir,
    const std::string& destinationVirtualFolder,
    AssetImportSubsystem* importer,
    const VirtualPathMounter& pathMounter,
    FAssetImportResult& outResult) const;

protected:
    bool OnImport(const FAssetImportRequest &request, const VirtualPathMounter &pathMounter,
        const std::string &destinationVirtualPath, const std::string &destinationPhysicalPath,
        FAssetImportResult &outResult) const override;

public:
    [[nodiscard]] std::string GetImporterName() const override { return "ModelImporter"; }
    [[nodiscard]] std::vector<EAssetType> GetOutputAssetTypes() const override
    {
        return {EAssetType::StaticMesh, EAssetType::Material, EAssetType::SkeletalMesh};
    }
    [[nodiscard]] std::vector<std::string> GetSupportedSourceExtensions() const override;
};
