// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "EAssetDomain.h"
#include "FAssetHeader.h"

struct FAssetRecord
{
    std::string assetID;
    std::string assetName;

    EAssetType assetType = EAssetType::Unknown;
    EAssetEncoding encoding = EAssetEncoding::Json;

    int32_t containerVersion = 1;
    int32_t payloadVersion = 1;

    std::string virtualPath;   // e.g. /Project/Props/Barrel.jasset
    std::string physicalPath;  // absolute normalized path to the .jasset file

    std::string sourcePath;    // optional
    std::string importerName;  // optional

    EAssetDomain domain;

    std::vector<std::string> dependencyAssetIDs;
};