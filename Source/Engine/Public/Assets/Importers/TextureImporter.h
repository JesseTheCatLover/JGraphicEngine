//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "IAssetImporter.h"

struct FAssetImportResult;

/**
 * @class TextureImporter
 * @brief Imports image source files into Texture2D .jasset files.
 *
 * Supports common 2D image formats such as png and jpg, and produces a single
 * Texture2D asset file per imported source image.
 */
class TextureImporter : public IAssetImporter
{
public:
    [[nodiscard]] std::string GetImporterName() const override { return "TextureImporter"; }
    [[nodiscard]] EAssetType GetOutputAssetType() const override { return EAssetType::Texture2D; }
    [[nodiscard]] std::vector<std::string> GetSupportedSourceExtensions() const override;

    bool Import(const FAssetImportRequest& request,
                const VirtualPathMounter& pathMounter,
                FAssetImportResult& outResult) const override;
};