// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "FAssetBrowserNode.h"
#include "Core/Memory/SmartPointers.h"

// Forward decl
template <typename T> class TSelectionModel;

enum class EAssetBrowserSelectionPolicy
{
    SharedGlobalSelection, // uses SelectionService asset-path selection
    LocalSelection         // view owns its own TSelectionModel<std::string>
};

enum class EAssetBrowserProjectionMode : uint8_t
{
    Flat,   // only immediate children of currentPath
    Tree    // recursive tree from a rootPath
};
struct FAssetBrowserViewState
{
    // For compilation of unique pointer TClass
    FAssetBrowserViewState();
    ~FAssetBrowserViewState();
    FAssetBrowserViewState(FAssetBrowserViewState&&) noexcept;
    FAssetBrowserViewState& operator=(FAssetBrowserViewState&&) noexcept;
    FAssetBrowserViewState(const FAssetBrowserViewState&) = delete;
    FAssetBrowserViewState& operator=(const FAssetBrowserViewState&) = delete;

    // Per-view navigation/filter state
    std::string currentPath = "/Project";
    std::string searchFilter;
    bool bDirty = true;

    // View preferences
    bool bShowFolders = true;
    bool bShowAssets  = true;

    EAssetBrowserProjectionMode projectionMode = EAssetBrowserProjectionMode::Flat;
    std::string rootPath = "/Project";      // tree root
    bool bIncludeRootNode = true;           // show "/Project" as a selectable node

    uint64_t sourceGraphVersion = 0;

    // Projected cache for this view instance
    std::unordered_map<AssetBrowserNodeID, FAssetBrowserNode> nodeCache;
    std::unordered_map<AssetBrowserNodeID, std::vector<AssetBrowserNodeID>> children;
    std::vector<AssetBrowserNodeID> viewNodeIDs;

    // Visible order for shift-click range selection (paths in the same order as viewNodeIDs)
    std::vector<std::string> visibleVirtualPaths;

    // Path -> ID mapping for reverse lookups (per-view)
    std::unordered_map<std::string, AssetBrowserNodeID> pathToID;

    // Track expanded folders
    std::unordered_set<AssetBrowserNodeID> expandedFolderNodes;

    // Selection policy (per-view)
    EAssetBrowserSelectionPolicy selectionPolicy = EAssetBrowserSelectionPolicy::SharedGlobalSelection;

    // Only used if selectionPolicy == LocalSelection
    TUniquePtr<TSelectionModel<std::string>> localSelectionModel;
};
