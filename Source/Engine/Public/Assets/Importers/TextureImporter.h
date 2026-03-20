//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "AssetImporterBase.h"

struct FAssetImportResult;

/**
 * @class TextureImporter
 * @brief Imports image source files into Texture2D .jasset files.
 *
 * Supports common 2D image formats such as png and jpg, and produces a single
 * Texture2D asset file per imported source image.
 */
class TextureImporter : public AssetImporterBase
{
public:
    bool OnImport(const FAssetImportRequest& request, const VirtualPathMounter& pathMounter,
      const std::string& destinationVirtualPath, const std::string& destinationPhysicalPath,
      FAssetImportResult& outResult) const override;

    [[nodiscard]] std::string GetImporterName() const override { return "TextureImporter"; }
    [[nodiscard]] EAssetType GetOutputAssetType() const override { return EAssetType::Texture2D; }
    [[nodiscard]] std::vector<std::string> GetSupportedSourceExtensions() const override;
};