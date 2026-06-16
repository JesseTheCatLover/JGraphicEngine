//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "Assets/AssetTypes.h"
#include "Assets/EAssetDomain.h"
#include "Assets/FAssetRecord.h"

enum class EAssetBrowserItemType
{
    Directory,
    Asset
};

struct FAssetBrowserDirectory
{
    std::string name;        // "Textures"
    std::string virtualPath; // "/Project/Textures"
};

struct FAssetBrowserAsset
{
    std::string name;        // "Wood"
    std::string virtualPath; // "/Project/Textures/Wood.jasset"

    std::string assetID;
    EAssetType type;
    EAssetDomain domain;

    const FAssetRecord* record = nullptr; // points to registry entry
};

struct FAssetBrowserDocument
{
    std::string currentPath = "/Project";

    std::vector<FAssetBrowserDirectory> directories;
    std::vector<FAssetBrowserAsset> assets;
};
