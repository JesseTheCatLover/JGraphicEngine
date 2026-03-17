//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "Assets/FAssetImportRequest.h"
#include "Assets/FAssetImportResult.h"
#include "Assets/AssetTypes.h"

class VirtualPathMounter;

/**
 * @class IAssetImporter
 * @brief Interface for importing external source files into engine-native asset files.
 *
 * An importer consumes a physical source file and produces one or more .jasset files
 * under a destination virtual folder in /Project.
 */
class IAssetImporter
{
public:
    virtual ~IAssetImporter() = default;

    /** @return Human-readable importer name, e.g. "TextureImporter". */
    [[nodiscard]] virtual std::string GetImporterName() const = 0;

    /** @return Primary asset type produced by this importer. */
    [[nodiscard]] virtual EAssetType GetOutputAssetType() const = 0;

    /** @return Supported source extensions without dot, e.g. png, jpg. */
    [[nodiscard]] virtual std::vector<std::string> GetSupportedSourceExtensions() const = 0;

    /**
     * @brief Import a source file into engine-native asset file(s).
     *
     * @param request Import request describing source path and destination virtual folder.
     * @param pathMounter Mounted virtual path authority used to resolve /Project destinations.
     * @param outResult Filled with created assets, warnings, and errors.
     * @return true if the import succeeded, false otherwise.
     */
    virtual bool Import(const FAssetImportRequest& request,
                        const VirtualPathMounter& pathMounter,
                        FAssetImportResult& outResult) const = 0;
};