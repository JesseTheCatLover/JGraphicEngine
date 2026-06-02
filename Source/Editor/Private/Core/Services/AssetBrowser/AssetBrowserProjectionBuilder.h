// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "FAssetBrowserNode.h"

struct FAssetBrowserViewSettings;
class AssetBrowserService;
struct FAssetBrowserViewProjection;

class AssetBrowserProjectionBuilder
{
public:
    static void Build(AssetBrowserService& service, const FAssetBrowserViewSettings& settings, FAssetBrowserViewProjection& view);

private:
    static void BuildFlat(AssetBrowserService& service, const FAssetBrowserViewSettings& settings, FAssetBrowserViewProjection& view);

    static void BuildTree(AssetBrowserService& service, const FAssetBrowserViewSettings& settings, FAssetBrowserViewProjection& view);
    static void BuildTreeRecursive(AssetBrowserService& service,const FAssetBrowserViewSettings& settings,
        FAssetBrowserViewProjection& view, AssetBrowserNodeID nodeID);

    static void AddNodeToView(FAssetBrowserViewProjection& view, const FAssetBrowserNode& node);
    static bool ShouldIncludeNode(const FAssetBrowserNode& node, const FAssetBrowserViewSettings& settings);
};