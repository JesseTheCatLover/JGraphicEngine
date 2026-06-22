// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <unordered_set>
#include "EditorCore/Services/AssetBrowser/FAssetBrowserViewProjection.h"
#include <EditorCore/Services/AssetBrowser/FAssetBrowserNode.h>


struct FAssetBrowserOutput
{
    bool bValid = false;

    FAssetBrowserViewProjection treeView;
    FAssetBrowserViewProjection contentView;

    std::string currentTreeNavigationPath;
    std::string currentContentNavigationPath;

    bool bCanNavigateBack = false;
    bool bCanNavigateForward = false;

    std::string currentTreeSearch;
    std::string currentContentSearch;

    std::unordered_set<AssetBrowserNodeID> selectedContentNodes;
    std::unordered_set<AssetBrowserNodeID> selectedTreeNodes;
};