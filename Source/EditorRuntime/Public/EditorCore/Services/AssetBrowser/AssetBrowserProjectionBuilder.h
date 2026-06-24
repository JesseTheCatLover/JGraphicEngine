// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "FAssetBrowserNode.h"

struct FAssetBrowserViewSettings;
class AssetBrowserViewController;
class AssetBrowserService;
struct FAssetBrowserViewProjection;

class AssetBrowserProjectionBuilder
{
public:
    static void Build(AssetBrowserService& service, const AssetBrowserViewController& controller, FAssetBrowserViewProjection& view);

private:
    static void BuildFlat(AssetBrowserService& service, const AssetBrowserViewController& controller, FAssetBrowserViewProjection& view);

    static void BuildTree(AssetBrowserService& service, const AssetBrowserViewController& controller, FAssetBrowserViewProjection& view);
    static void BuildTreeRecursive(AssetBrowserService& service,const AssetBrowserViewController& controller,
        FAssetBrowserViewProjection& view, AssetBrowserNodeID nodeID);

    static bool ShouldIncludeNode(const FAssetBrowserNode& node, const FAssetBrowserViewSettings &settings);

    static void RegisterNode(FAssetBrowserViewProjection& view, const FAssetBrowserNode& node);
    static void AddVisibleNodeForFlat(FAssetBrowserViewProjection& view, const FAssetBrowserNode& node);
    static void AddVisibleNodeForTree(FAssetBrowserViewProjection& view, const FAssetBrowserNode& node);
};