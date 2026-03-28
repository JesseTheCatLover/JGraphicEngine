//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "AssetImporterBase.h"
#include "IAssetImporter.h"

class ModelImporter : public AssetImporterBase
{
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
