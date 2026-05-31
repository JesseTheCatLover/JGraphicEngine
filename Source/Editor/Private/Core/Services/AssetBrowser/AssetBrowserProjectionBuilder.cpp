//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetBrowserProjectionBuilder.h"

#include <iostream>
#include <unordered_set>

#include "AssetBrowserService.h"

void AssetBrowserProjectionBuilder::Build(
    AssetBrowserService& service,
    FAssetBrowserViewState& view)
{
    switch (view.projectionMode)
    {
        case EAssetBrowserProjectionMode::Flat:
            BuildFlat(service, view);
            break;

        case EAssetBrowserProjectionMode::Tree:
            BuildTree(service, view);
            break;
    }
}

void AssetBrowserProjectionBuilder::BuildFlat(
    AssetBrowserService& service,
    FAssetBrowserViewState& view)
{
    const AssetBrowserNodeID rootID = service.GetRootNodeID(view.currentPath);

    service.SyncFolderNode(rootID);

    const auto* rootNode = service.TryGetModelNode(rootID);

    if (!rootNode)
        return;

    view.nodeCache[rootID] = *rootNode;
    view.pathToID[rootNode->virtualPath] = rootID;

    const auto& children = service.GetChildren(rootID);

    for (AssetBrowserNodeID childID : children)
    {
        const auto* child = service.TryGetModelNode(childID);

        if (!child)
            continue;

        if (child->type == EAssetBrowserNodeType::Folder && !view.bShowFolders)
            continue;

        if (child->type == EAssetBrowserNodeType::Asset && !view.bShowAssets)
            continue;

        view.nodeCache[childID] = *child;
        view.pathToID[child->virtualPath] = childID;
        view.viewNodeIDs.push_back(childID);
        view.visibleVirtualPaths.push_back(child->virtualPath);
    }
}

void AssetBrowserProjectionBuilder::BuildTree(AssetBrowserService &service, FAssetBrowserViewState &view)
{
    const AssetBrowserNodeID rootID = service.GetRootNodeID(view.rootPath);

    service.SyncFolderNode(rootID);

    BuildTreeRecursive(service, view, rootID);
}

void AssetBrowserProjectionBuilder::BuildTreeRecursive(AssetBrowserService &service, FAssetBrowserViewState &view,
    AssetBrowserNodeID nodeID)
{
    const FAssetBrowserNode* node = service.TryGetModelNode(nodeID);

    if (!node)
        return;

    const bool bIsFolder = node->type == EAssetBrowserNodeType::Folder;
    const bool bExpanded = bIsFolder && view.expandedFolderNodes.contains(nodeID);
    std::cout
        << "Node: " << node->virtualPath
        << " Expanded=" << bExpanded
        << std::endl;
    if (bExpanded)
    {
        service.SyncFolderNode(nodeID); // Sync the model first then copy
        node = service.TryGetModelNode(nodeID); // Reacquire the pointer

        if (!node)
            return;
    }

    view.nodeCache[nodeID] = *node;
    view.pathToID[node->virtualPath] = nodeID;
    view.viewNodeIDs.push_back(nodeID);
    view.visibleVirtualPaths.push_back(node->virtualPath);

    if (!bIsFolder)
        return; // Assets don't have children, we quit

    if (!bExpanded)
        return; // We don't care about collapsed folder's children


    const auto& children = service.GetChildren(nodeID);
    view.children[nodeID] = children;

    for (AssetBrowserNodeID childID : children)
    {
        const FAssetBrowserNode* child = service.TryGetModelNode(childID);

        if (!child)
            continue;

        if (child->type == EAssetBrowserNodeType::Folder && !view.bShowFolders)
        {
            continue;
        }

        if (child->type == EAssetBrowserNodeType::Asset &&
            !view.bShowAssets)
        {
            continue;
        }

        BuildTreeRecursive(service, view, childID);
    }
}
