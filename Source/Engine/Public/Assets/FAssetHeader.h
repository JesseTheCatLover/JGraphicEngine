// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "AssetTypes.h"

struct FAssetHeader
{
    static constexpr int32_t CurrentContainerVersion = 1;

    int32_t containerVersion = CurrentContainerVersion;
    int32_t payloadVersion = 1;

    EAssetType assetType = EAssetType::Unknown;
    EAssetEncoding encoding = EAssetEncoding::Json;

    std::string assetID;

    std::string sourcePath;     // optional
    std::string importerName;   // optional

    std::vector<std::string> dependencyAssetIDs;
};