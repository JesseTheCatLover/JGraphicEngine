// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "FAssetBrowserNode.h"

struct FAssetBrowserViewProjection
{
    // Projected cache for this view instance
    std::unordered_map<AssetBrowserNodeID, FAssetBrowserNode> nodeCache;
    std::unordered_map<AssetBrowserNodeID, std::vector<AssetBrowserNodeID>> children;
    std::vector<AssetBrowserNodeID> viewNodeIDs; ///< NOTE: Unlike Flat view, For Tree view only the root node is pushed into this list

    // Visible order for shift-click range selection (paths in the same order as viewNodeIDs)
    std::vector<std::string> visibleVirtualPaths;

    // Path -> ID mapping for reverse lookups (per-view)
    std::unordered_map<std::string, AssetBrowserNodeID> pathToID;

void Clear()
    {
        nodeCache.clear();
        pathToID.clear();
        children.clear();
        viewNodeIDs.clear();
        visibleVirtualPaths.clear();
    }
};
