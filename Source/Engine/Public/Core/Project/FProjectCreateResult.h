// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

struct FProjectCreateResult
{
    bool bSuccess = false;

    std::string projectRootPath;
    std::string projectFilePath;

    std::vector<std::string> errors;
};