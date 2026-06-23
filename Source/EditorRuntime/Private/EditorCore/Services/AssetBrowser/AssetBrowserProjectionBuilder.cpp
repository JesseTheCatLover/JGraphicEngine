//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorCore/Services/AssetBrowser/AssetBrowserProjectionBuilder.h"
#include "EditorCore/Services/AssetBrowser/AssetBrowserService.h"
#include "EditorCore/Services/AssetBrowser/AssetBrowserViewController.h"
#include "EditorCore/Services/AssetBrowser/FAssetBrowserViewSettings.h"

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

    if (controller.GetSettings().bIncludeRootNode)
    {
        view.viewNodeIDs.push_back(rootID);
        BuildTreeRecursive(service, controller, view, rootID);
    }
    else
    {
        const auto& children = service.GetChildren(rootID);

        for (auto childID : children)
        {
            view.viewNodeIDs.push_back(childID); // ONLY roots here
            BuildTreeRecursive(service, controller, view, childID);
        }
    }
}

void AssetBrowserProjectionBuilder::BuildTreeRecursive(AssetBrowserService &service, const AssetBrowserViewController& controller,
    FAssetBrowserViewProjection &view, AssetBrowserNodeID nodeID)
{
    const FAssetBrowserNode* node = service.TryGetModelNode(nodeID);
    if (!node) return;

    RegisterNode(view, *node);

    const bool bIsFolder = node->type == EAssetBrowserNodeType::Folder;
    const bool bExpanded = bIsFolder && controller.IsFolderExpanded(nodeID);

    if (!bIsFolder || !bExpanded)
        return;

    const auto& modelChildren = service.GetChildren(nodeID);

    for (auto childID : modelChildren)
    {
        const FAssetBrowserNode* child = service.TryGetModelNode(childID);
        if (!child) continue;
        if (!ShouldIncludeNode(*child, controller.GetSettings())) continue;

        view.children[nodeID].push_back(childID);

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

void AssetBrowserProjectionBuilder::RegisterNode(FAssetBrowserViewProjection &view, const FAssetBrowserNode &node)
{
    view.nodeCache[node.nodeID] = node;
    view.pathToID[node.virtualPath] = node.nodeID;
}
