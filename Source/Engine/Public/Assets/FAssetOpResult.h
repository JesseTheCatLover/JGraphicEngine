//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>

struct FAssetOpResult
{
    bool bSuccess = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> affectedVirtualFolders;
};
