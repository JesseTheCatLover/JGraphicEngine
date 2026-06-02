//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetBrowserProjectionBuilder.h"
#include "AssetBrowserService.h"
#include "AssetBrowserViewController.h"
#include "FAssetBrowserViewSettings.h"

void AssetBrowserProjectionBuilder::Build(AssetBrowserService& service, const AssetBrowserViewController& controller,
                                          FAssetBrowserViewProjection& view)
{
    switch (controller.GetSettings().projectionMode)
    {
        case EAssetBrowserProjectionMode::Flat:
            BuildFlat(service, controller, view);
            break;

        case EAssetBrowserProjectionMode::Tree:
            BuildTree(service, controller, view);
            break;
    }
}

void AssetBrowserProjectionBuilder::BuildFlat(AssetBrowserService& service, const AssetBrowserViewController& controller,
    FAssetBrowserViewProjection& view)
{
    const AssetBrowserNodeID rootID = service.GetRootNodeID(controller.GetSettings().currentPath);

    service.EnsureFolderLoaded(rootID);

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

        if (!ShouldIncludeNode(*child, controller.GetSettings()))
            continue;

        AddNodeToView(view, *child);
    }
}

void AssetBrowserProjectionBuilder::BuildTree(AssetBrowserService& service,
    const AssetBrowserViewController& controller, FAssetBrowserViewProjection& view)
{
    const AssetBrowserNodeID rootID = service.GetRootNodeID(controller.GetSettings().rootPath);

    service.EnsureFolderLoaded(rootID);

    if (controller.GetSettings().bIncludeRootNode) // If root should be included we include it
    {
        BuildTreeRecursive(service, controller, view, rootID);
        return;
    }

    const auto& children = service.GetChildren(rootID); // If not we start from its children

    for (AssetBrowserNodeID childID : children)
    {
        const FAssetBrowserNode* child = service.TryGetModelNode(childID);

        if (!child)
            continue;

        if (!ShouldIncludeNode(*child, controller.GetSettings()))
            continue;

        BuildTreeRecursive(service, controller, view, childID);
    }
}

void AssetBrowserProjectionBuilder::BuildTreeRecursive(AssetBrowserService &service, const AssetBrowserViewController& controller,
    FAssetBrowserViewProjection &view, AssetBrowserNodeID nodeID)
{
    const FAssetBrowserNode* node = service.TryGetModelNode(nodeID);

    if (!node)
        return;

    const bool bIsFolder = node->type == EAssetBrowserNodeType::Folder;
    const bool bExpanded = bIsFolder && controller.IsFolderExpanded(nodeID);

    if (bExpanded)
    {
        service.EnsureFolderLoaded(nodeID); // Load the folder and children for model first then copy
        node = service.TryGetModelNode(nodeID); // Reacquire the pointer

        if (!node)
            return;
    }

    AddNodeToView(view, *node);

    if (!bIsFolder)
        return; // Assets don't have children, we quit

    if (!bExpanded)
        return; // We don't care about collapsed folder's children

    const auto& modelChildren = service.GetChildren(nodeID);

    auto& projectedChildren = view.children[nodeID];

    for (AssetBrowserNodeID childID : modelChildren)
    {
        const FAssetBrowserNode* child = service.TryGetModelNode(childID);

        if (!child)
            continue;

        if (!ShouldIncludeNode(*child, controller.GetSettings()))
            continue;

        projectedChildren.push_back(childID);

        BuildTreeRecursive(service, controller, view, childID);
    }
}

void AssetBrowserProjectionBuilder::AddNodeToView(FAssetBrowserViewProjection &view, const FAssetBrowserNode &node)
{
    view.nodeCache[node.nodeID] = node;
    view.pathToID[node.virtualPath] = node.nodeID;
    view.viewNodeIDs.push_back(node.nodeID);
    view.visibleVirtualPaths.push_back(node.virtualPath);
}

bool AssetBrowserProjectionBuilder::ShouldIncludeNode(const FAssetBrowserNode &node,
    const FAssetBrowserViewSettings &settings)
{
    if (node.type == EAssetBrowserNodeType::Folder)
        return settings.bShowFolders;

    if (node.type == EAssetBrowserNodeType::Asset)
        return settings.bShowAssets;

    return true;
}
