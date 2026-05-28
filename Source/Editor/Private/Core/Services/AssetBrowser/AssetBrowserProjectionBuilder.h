// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

class AssetBrowserService;
struct FAssetBrowserViewState;

class AssetBrowserProjectionBuilder
{
public:
    static void Build(AssetBrowserService& service, FAssetBrowserViewState& view);
};