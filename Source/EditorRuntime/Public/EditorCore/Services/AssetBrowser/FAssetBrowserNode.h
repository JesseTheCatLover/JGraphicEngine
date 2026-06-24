// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "Assets/AssetTypes.h"
#include "Assets/EAssetDomain.h"
#include "Utilities/UPath.h"

enum class EAssetBrowserNodeType
{
    Folder,
    Asset
};

enum class EAssetBrowserChildState : uint8_t
{
    Unknown,
    Present,
    None
};

// Typedef for fast lookup hashes
using AssetBrowserNodeID = uint64_t;

struct FAssetBrowserNode
{
    AssetBrowserNodeID nodeID = 0;    // Stable graph node ID
    AssetBrowserNodeID parentID = 0;  // Parent node ID (0 = none)

    EAssetBrowserNodeType type = EAssetBrowserNodeType::Folder;

    std::string virtualPath; // e.g. "/Project/Textures/Sword_BaseColor"

    EAssetBrowserChildState childFolderState = EAssetBrowserChildState::Unknown;
    EAssetBrowserChildState childAssetState = EAssetBrowserChildState::Unknown;

    [[nodiscard]] std::string GetDisplayName() const
    {
        if (virtualPath == "/Project") return "Project"; // Root display name polish
        if (virtualPath == "/Engine") return "Engine";
        return UPath::GetFileName(virtualPath, false);
    }

    [[nodiscard]] bool HasFolderChildren() const
    {
        return childFolderState == EAssetBrowserChildState::Present;
    }

    [[nodiscard]] bool HasAssetChildren() const
    {
        return childAssetState == EAssetBrowserChildState::Present;
    }

    [[nodiscard]] bool HasAnyChildren() const
    {
        return childFolderState == EAssetBrowserChildState::Present ||
               childAssetState == EAssetBrowserChildState::Present;
    }

    [[nodiscard]] bool FolderChildrenKnown() const
    {
        return childFolderState != EAssetBrowserChildState::Unknown;
    }

    [[nodiscard]] bool AssetChildrenKnown() const
    {
        return childAssetState != EAssetBrowserChildState::Unknown;
    }

    // Asset-only
    std::string assetID; // registry UUID
    EAssetType assetType = EAssetType::Unknown;
    EAssetDomain domain = EAssetDomain::Project;
};
