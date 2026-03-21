//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "AssetImporterBase.h"
#include "IAssetImporter.h"

class StaticMeshImporter : public AssetImporterBase
{
protected:
    bool OnImport(const FAssetImportRequest &request, const VirtualPathMounter &pathMounter,
        const std::string &destinationVirtualPath, const std::string &destinationPhysicalPath,
        FAssetImportResult &outResult) const override;

public:
    [[nodiscard]] std::string GetImporterName() const override { return "StaticMeshImporter"; }
    [[nodiscard]] EAssetType GetOutputAssetType() const override { return EAssetType::StaticMesh; }
    [[nodiscard]] std::vector<std::string> GetSupportedSourceExtensions() const override;
};
