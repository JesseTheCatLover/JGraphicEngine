// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>
#include <unordered_map>

struct FAssetOpResult
{
    bool bSuccess = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> affectedVirtualFolders; ///< Folders whose contents might need a refresh

    // --- For Rename/Move operations ---

    // Maps old path to new path. Only populated for operations that can remap paths.
    // Key = old path, Value = new path
    std::unordered_map<std::string, std::string> pathRemappings;

    // For delete operations: list of paths that were deleted
    std::vector<std::string> deletedPaths;
};
