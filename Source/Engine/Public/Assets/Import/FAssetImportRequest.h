//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

struct FAssetImportRequest
{
    std::string sourceFilePath;           // absolute or resolvable source path
    std::string destinationVirtualFolder; // e.g. "/Project/Textures"
    bool bReplaceExisting = false;
};