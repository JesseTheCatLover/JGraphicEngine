//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetBrowserViewController.h"
#include "Core/Services/Selection/TSelectionModel.h"
#include "Utilities/UPath.h"

AssetBrowserViewController::AssetBrowserViewController() = default;
AssetBrowserViewController::~AssetBrowserViewController() = default;
AssetBrowserViewController::AssetBrowserViewController(AssetBrowserViewController&&) noexcept = default;
AssetBrowserViewController& AssetBrowserViewController::operator=(AssetBrowserViewController&&) noexcept = default;

void AssetBrowserViewController::RequestNavigateTo(const std::string& path)
{
    m_PendingCurrentPath = UPath::Normalize(path);
}

void AssetBrowserViewController::RequestSetRoot(const std::string &path)
{
    m_PendingRootPath = UPath::Normalize(path);
}

void AssetBrowserViewController::RequestIncludeRootNode(bool value)
{
    m_PendingIncludeRootNode = value;
}

void AssetBrowserViewController::RequestExpandAll()
{
    m_bExpandAll = true;
    m_bCollapseAll = false;
}

void AssetBrowserViewController::RequestCollapseAll()
{
    m_bCollapseAll = true;
    m_bExpandAll = false;
}

void AssetBrowserViewController::RequestSetSearchFilter(std::string filter)
{
    m_PendingSearchFilter = std::move(filter);
}

void AssetBrowserViewController::RequestShowFolders(bool value)
{
    m_PendingShowFolders = value;
}

void AssetBrowserViewController::RequestShowAssets(bool value)
{
    m_PendingShowAssets = value;
}

void AssetBrowserViewController::RequestProjectionMode(EAssetBrowserProjectionMode mode)
{
    m_PendingProjectionMode = mode;
}

void AssetBrowserViewController::RequestSelectNode(AssetBrowserNodeID id, bool bToggle, bool bRange)
{
    m_PendingSelections.push_back({id, bToggle, bRange});
}

void AssetBrowserViewController::RequestClearSelection()
{
    m_bClearSelection = true;
}

void AssetBrowserViewController::RequestRefresh()
{
    m_bProjectionDirty = true;
}

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

void AssetBrowserViewController::Flush(AssetBrowserService& service, FAssetBrowserViewProjection& view)
{
    ApplyPendingSettings();
    ApplyPendingExpansion(service);

    if (m_bProjectionDirty)
    {
        service.RefreshView(m_Settings, view);
        m_bProjectionDirty = false;
    }

    ApplyPendingSelection(service, view);

    ClearPendingCommands();
}

void AssetBrowserViewController::Clear()
{
    m_Settings.expandedFolderNodes.clear();

    if (m_LocalSelectionModel)
        m_LocalSelectionModel->Clear();

    ClearPendingCommands();

    m_bProjectionDirty = true;
}

TSelectionModel<std::string> & AssetBrowserViewController::GetSelectionModel(AssetBrowserService &service)
{
    if (m_Settings.selectionPolicy == EAssetBrowserSelectionPolicy::SharedGlobalSelection)
    {
        return service.GetGlobalSelectionModel();
    }

    if (!m_LocalSelectionModel)
        m_LocalSelectionModel = MakeUnique<TSelectionModel<std::string>>();

    return *m_LocalSelectionModel;
}

const TSelectionModel<std::string> & AssetBrowserViewController::GetSelectionModel(
    const AssetBrowserService &service) const
{
    if (m_Settings.selectionPolicy == EAssetBrowserSelectionPolicy::SharedGlobalSelection)
    {
        return service.GetGlobalSelectionModel();
    }

    static TSelectionModel<std::string> s_Empty;
    return m_LocalSelectionModel ? *m_LocalSelectionModel : s_Empty;
}

void AssetBrowserViewController::ApplyPendingSettings()
{
    if (m_PendingCurrentPath)
    {
        m_Settings.currentPath = *m_PendingCurrentPath;
        m_bProjectionDirty = true;
    }

    if (m_PendingRootPath)
    {
        m_Settings.rootPath = *m_PendingRootPath;
        m_bProjectionDirty = true;
    }

    if (m_PendingIncludeRootNode)
    {
        m_Settings.bIncludeRootNode = *m_PendingIncludeRootNode;
        m_bProjectionDirty = true;
    }

    if (m_PendingSearchFilter)
    {
        m_Settings.searchFilter = std::move(*m_PendingSearchFilter);
        m_bProjectionDirty = true;
    }

    if (m_PendingShowFolders)
    {
        m_Settings.bShowFolders = *m_PendingShowFolders;
        m_bProjectionDirty = true;
    }

    if (m_PendingShowAssets)
    {
        m_Settings.bShowAssets = *m_PendingShowAssets;
        m_bProjectionDirty = true;
    }

    if (m_PendingProjectionMode)
    {
        m_Settings.projectionMode = *m_PendingProjectionMode;
        m_bProjectionDirty = true;
    }
}

void AssetBrowserViewController::ApplyPendingExpansion(const AssetBrowserService& service)
{
    if (m_bCollapseAll)
    {
        if (!m_Settings.expandedFolderNodes.empty())
        {
            m_Settings.expandedFolderNodes.clear();
            m_bProjectionDirty = true;
        }
    }

    if (m_bExpandAll)
    {
        bool bChanged = false;

        for (AssetBrowserNodeID id : service.GetAllFolderIDs())
        {
            bChanged |= m_Settings.expandedFolderNodes.insert(id).second;
        }

        if (bChanged)
            m_bProjectionDirty = true;
    }

    for (AssetBrowserNodeID id : m_PendingExpand)
    {
        if (m_Settings.expandedFolderNodes.insert(id).second)
        {
            m_bProjectionDirty = true;
        }
    }

    for (AssetBrowserNodeID id : m_PendingCollapse)
    {
        if (m_Settings.expandedFolderNodes.erase(id) > 0)
        {
            m_bProjectionDirty = true;
        }
    }
}

void AssetBrowserViewController::ApplyPendingSelection(AssetBrowserService& service,
    const FAssetBrowserViewProjection& view)
{
    auto& selection = GetSelectionModel(service);

    if (m_bClearSelection)
        selection.Clear();

    for (const FSelectionRequest& request : m_PendingSelections)
    {
        const FAssetBrowserNode* node = service.GetNode(view, request.id);

        if (!node)
            continue;

        FSelectionModifiers mods;
        mods.bToggle = request.bToggle;
        mods.bRange = request.bRange;

        selection.ApplyClick(node->virtualPath, mods, &view.visibleVirtualPaths);
    }
}

void AssetBrowserViewController::ClearPendingCommands()
{
    m_PendingCurrentPath.reset();
    m_PendingRootPath.reset();

    m_PendingIncludeRootNode.reset();

    m_PendingSearchFilter.reset();

    m_PendingShowFolders.reset();
    m_PendingShowAssets.reset();

    m_PendingProjectionMode.reset();

    m_bExpandAll = false;
    m_bCollapseAll = false;

    m_PendingExpand.clear();
    m_PendingCollapse.clear();

    m_PendingSelections.clear();

    m_bClearSelection = false;
}
