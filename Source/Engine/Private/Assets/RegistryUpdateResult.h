//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

struct FRegistryIssue
{
    std::string key;      // assetID
    std::string message;  // human readable
};

struct FRegistryUpdateResult
{
    bool bSuccess = true; // becomes false if any operation fails

    int removed = 0;
    int added = 0;
    int updated = 0;
    int failed = 0;

    std::vector<FRegistryIssue> issues;

    void AddFailure(std::string key, std::string msg)
    {
        bSuccess = false;
        failed++;
        issues.push_back({std::move(key), std::move(msg)});
    }
};
