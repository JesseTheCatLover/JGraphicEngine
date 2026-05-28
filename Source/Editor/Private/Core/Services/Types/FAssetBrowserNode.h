// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "Assets/AssetTypes.h"
#include "Assets/EAssetDomain.h"

enum class EAssetBrowserNodeType
{
    Folder,
    Asset
};

// Typedef for fast lookup hashes
using AssetBrowserNodeID = uint64_t;

struct FAssetBrowserNode
{
    AssetBrowserNodeID nodeID = 0;    // Hash of VirtualPath
    AssetBrowserNodeID parentID = 0;  // Hash of parent folder (0 if root/unknown)

    EAssetBrowserNodeType type = EAssetBrowserNodeType::Folder;

    std::string displayName; // e.g. "Textures" or "Sword_BaseColor"
    std::string virtualPath; // e.g. "/Project/Textures/Sword_BaseColor"

    // Folder-only hints for UI tree rendering
    bool bChildFoldersKnown = false;
    bool bHasChildFolders = false;

    bool bChildAssetsKnown = false;
    bool bHasChildAssets = false;

    // Asset-only
    std::string assetID;              // registry UUID
    EAssetType assetType = EAssetType::Unknown;
    EAssetDomain domain = EAssetDomain::Project;
};
