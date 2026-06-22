// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include "Panels/Controllers/Inputs/FPanelInputBase.h"
#include <EditorCore/Services/AssetBrowser/FAssetBrowserNode.h>

struct FAssetBrowserPanelInput : FPanelInputBase
{
    // Navigation

    bool bNavigateToPath = false;
    std::string navigateToPath;

    bool bNavigatePrevious = false;
    bool bNavigateNext = false;
    bool bNavigateUp = false;
    bool bNavigateHome = false;

    // Search

    bool bSetTreeSearch = false;
    std::string treeSearch;

    bool bSetContentSearch = false;
    std::string contentSearch;

    // Tree expansion

    std::vector<AssetBrowserNodeID> expandNodes;
    std::vector<AssetBrowserNodeID> collapseNodes;

    // Selection

    struct FNodeSelection
    {
        AssetBrowserNodeID nodeID = 0;
        bool bRange = false;
        bool bToggle = false;
    };

    std::vector<FNodeSelection> treeSelections;
    std::vector<FNodeSelection> contentSelections;

    bool bClearTreeSelection = false;
    bool bClearContentSelection = false;

    // Open

    bool bOpenNode = false;
    AssetBrowserNodeID openNodeID = 0;

    // Mutations

    enum class EMutationType
    {
        None,
        Rename,
        Delete,
        Duplicate,
        Move,
        CreateFolder
    };

    struct FMutationRequest
    {
        EMutationType type;
        AssetBrowserNodeID nodeID;

        std::string text;
        std::string destinationPath;
    };

    std::vector<FMutationRequest> mutations;
};