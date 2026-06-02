//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>

using AssetBrowserNodeID = uint64_t;

enum class EAssetBrowserSelectionPolicy
{
    SharedGlobalSelection, // uses SelectionService asset-path selection
    LocalSelection         // controller owns its own TSelectionModel<std::string>
};

enum class EAssetBrowserProjectionMode : std::uint8_t
{
    Flat,   // only immediate children of currentPath
    Tree    // recursive tree from a rootPath
};

struct FAssetBrowserViewSettings
{
    // Selection policy (per-controller)
    EAssetBrowserSelectionPolicy selectionPolicy = EAssetBrowserSelectionPolicy::SharedGlobalSelection;

    std::string currentPath = "/Project";
    std::string rootPath = "/Project";

    std::string searchFilter;

    bool bShowFolders = true;
    bool bShowAssets = true;

    bool bIncludeRootNode = true;

    EAssetBrowserProjectionMode projectionMode = EAssetBrowserProjectionMode::Flat;

    std::unordered_set<AssetBrowserNodeID> expandedFolderNodes;
};
