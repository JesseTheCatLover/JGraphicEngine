// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include "Controllers/Inputs/FPanelInputBase.h"

struct FAssetBrowserPanelInput : FPanelInputBase
{
    // Navigation commands produced by the panel UI
    bool bNavigateToPath = false;
    std::string navigateToPath;      // "/Project/Textures"

    bool bNavigateUp = false;        // ".."
    bool bNavigateHome = false;      // optional: jump to "/Project" or "/"

    bool bForceRefresh = false;

    // Optional future UI state:
    // std::string searchQuery;
    // bool bShowEngineAssets = false;
};
