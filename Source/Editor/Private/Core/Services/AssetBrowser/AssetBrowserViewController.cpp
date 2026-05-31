//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetBrowserViewController.h"
#include "Core/Services/Selection/TSelectionModel.h"

AssetBrowserViewController::AssetBrowserViewController() = default;
AssetBrowserViewController::~AssetBrowserViewController() = default;
AssetBrowserViewController::AssetBrowserViewController(AssetBrowserViewController&&) noexcept = default;
AssetBrowserViewController& AssetBrowserViewController::operator=(AssetBrowserViewController&&) noexcept = default;

void AssetBrowserViewController::RequestExpand(AssetBrowserNodeID id)
{
    m_PendingExpand.insert(id);
    m_PendingCollapse.erase(id);
}

void AssetBrowserViewController::RequestCollapse(AssetBrowserNodeID id)
{
    m_PendingCollapse.insert(id);
    m_PendingExpand.erase(id);
}

void AssetBrowserViewController::Flush(AssetBrowserService& service, FAssetBrowserViewState& view)
{
    bool bChanged = false;

    for (AssetBrowserNodeID id : m_PendingExpand)
    {
        const bool before = service.IsFolderExpanded(view, id);
        service.ExpandFolderNode(view, id);
        const bool after = service.IsFolderExpanded(view, id);

        bChanged |= (before != after);
    }

    for (AssetBrowserNodeID id : m_PendingCollapse)
    {
        const bool before = service.IsFolderExpanded(view, id);
        service.CollapseFolderNode(view, id);
        const bool after = service.IsFolderExpanded(view, id);

        bChanged |= (before != after);
    }

    if (bChanged)
    {
        view.bDirty = true;
    }

    Clear();
}

void AssetBrowserViewController::Clear()
{
    m_PendingExpand.clear();
    m_PendingCollapse.clear();
}
