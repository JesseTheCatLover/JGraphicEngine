//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "Assets/AssetTypes.h"

/**
 * @struct FImportedAssetInfo
 * @brief Describes one asset file created by an import operation.
 */
struct FImportedAssetInfo
{
    std::string assetID;       ///< Stable asset identifier written into the imported .jasset.
    EAssetType assetType = EAssetType::Unknown; ///< Type of asset created.
    std::string virtualPath;   ///< Virtual path of the created asset file.
    std::string physicalPath;  ///< Physical filesystem path of the created asset file.
};

/**
 * @struct FAssetImportResult
 * @brief Result of an asset import operation.
 *
 * One source file may produce one or more engine asset files, so created assets are returned
 * as a list. Warnings and errors are accumulated for editor-facing reporting.
 */
struct FAssetImportResult
{
    bool bSuccess = false;
    std::vector<FImportedAssetInfo> createdAssets;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};