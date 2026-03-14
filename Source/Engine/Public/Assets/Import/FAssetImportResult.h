//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

struct FImportedAssetInfo
{
    std::string assetID;
    std::string virtualPath;
    std::string physicalPath;
};

struct FAssetImportResult
{
    bool bSuccess = false;
    std::vector<FImportedAssetInfo> createdAssets;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};