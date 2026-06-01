// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "FAssetBrowserNode.h"

struct FAssetBrowserViewProjection
{
    uint64_t sourceGraphVersion = 0;

    // Projected cache for this view instance
    std::unordered_map<AssetBrowserNodeID, FAssetBrowserNode> nodeCache;
    std::unordered_map<AssetBrowserNodeID, std::vector<AssetBrowserNodeID>> children;
    std::vector<AssetBrowserNodeID> viewNodeIDs;

    // Visible order for shift-click range selection (paths in the same order as viewNodeIDs)
    std::vector<std::string> visibleVirtualPaths;

    // Path -> ID mapping for reverse lookups (per-view)
    std::unordered_map<std::string, AssetBrowserNodeID> pathToID;
};
