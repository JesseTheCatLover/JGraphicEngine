//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

/**
 * @struct FAssetImportRequest
 * @brief Describes a request to import a source asset file into the project asset space.
 *
 * The source file is a physical filesystem path. The destination is a virtual folder
 * under /Project where the importer should place the generated .jasset file(s).
 */
struct FAssetImportRequest
{
    std::string sourceFilePath;           ///< Physical path to the original source file.
    std::string destinationVirtualFolder; ///< Destination folder under /Project, e.g. "/Project/Textures/UI".
    bool bOverwrite = false;              ///< Whether an existing destination asset may be overwritten.
};