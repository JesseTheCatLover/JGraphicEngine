// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "FAssetBrowserNode.h"

class AssetBrowserService;
struct FAssetBrowserViewState;

class AssetBrowserProjectionBuilder
{
public:
    static void Build(AssetBrowserService& service, FAssetBrowserViewState& view);

private:
    static void BuildFlat(AssetBrowserService& service, FAssetBrowserViewState& view);

    static void BuildTree(AssetBrowserService& service, FAssetBrowserViewState& view);
    static void BuildTreeRecursive(AssetBrowserService& service, FAssetBrowserViewState& view, AssetBrowserNodeID nodeID);
};