//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorCore/Services/AssetBrowser/AssetBrowserViewController.h"

#include <utility>
#include "EditorCore/Services/Selection/TSelectionModel.h"
#include "Utilities/UPath.h"

AssetBrowserViewController::AssetBrowserViewController(FAssetBrowserViewSettings initialSettings)
{
    m_Settings = std::move(initialSettings);

    if (m_Settings.selectionPolicy == EAssetBrowserSelectionPolicy::LocalSelection)
    {
        m_LocalSelectionModel = MakeUnique<TSelectionModel<std::string>>();
    }

    m_bControllerDirty = true;
}

AssetBrowserViewController::~AssetBrowserViewController() = default;

FDelegateHandle AssetBrowserViewController::AddProjectionModifier(FProjectionModifier modifier)
{
    const FDelegateHandle handle{m_ModifierIDGenerator.Allocate()};
    m_ProjectionModifiers.push_back({handle, std::move(modifier)});

    m_bControllerDirty = true;
    return handle;
}

bool AssetBrowserViewController::RemoveProjectionModifier(FDelegateHandle modifierHandle)
{
    const auto it = std::ranges::find_if(m_ProjectionModifiers,
                                   [&](const FProjectionModifierEntry& slot)
                                   {
                                       return slot.handle == modifierHandle;
                                   });

    if (it == m_ProjectionModifiers.end())
        return false;

    m_ModifierIDGenerator.Free(modifierHandle.id);
    m_ProjectionModifiers.erase(it);

    m_bControllerDirty = true;
    return true;
}

void AssetBrowserViewController::ClearAllProjectionModifiers()
{
    if (m_ProjectionModifiers.empty())
        return;

    m_ProjectionModifiers.clear();
    m_ModifierIDGenerator.Reset();

    m_bControllerDirty = true;
}

const std::string& AssetBrowserViewController::GetCurrentNavigationPath()
{
    return m_Settings.currentPath;
}

void AssetBrowserViewController::RequestNavigateTo(const std::string& path)
{
    m_PendingCurrentPath = UPath::NormalizeVirtual(path);
}

void AssetBrowserViewController::RequestSetRoot(const std::string &path)
{
    m_PendingRootPath = UPath::NormalizeVirtual(path);
}

void AssetBrowserViewController::RequestIncludeRootNode(bool value)
{
    m_PendingIncludeRootNode = value;
}

void AssetBrowserViewController::RequestExpandRoot(AssetBrowserService &service)
{
    RequestExpand(service.GetRootNodeID(m_Settings.rootPath));
}

void AssetBrowserViewController::RequestCollapseRoot(AssetBrowserService &service)
{
    RequestCollapse(service.GetRootNodeID(m_Settings.rootPath));
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

bool AssetBrowserViewController::IsFolderExpanded(AssetBrowserNodeID id) const
{
    return m_ExpandedNodes.contains(id);
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
    m_bControllerDirty = true;
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

void AssetBrowserViewController::Update(AssetBrowserService& service)
{
    ApplyPendingSettings();

    if (m_bControllerDirty)
    {
        service.RefreshView(*this, m_View);
        m_bControllerDirty = false;
    }

    ApplyProjectionModifiers();

    ApplyPendingExpansion(service);

    ApplyPendingSelection(service, m_View);

    ClearPendingCommands();
}

void AssetBrowserViewController::Clear()
{
    m_ExpandedNodes.clear();
    ClearAllProjectionModifiers();

    if (m_LocalSelectionModel)
        m_LocalSelectionModel->Clear();

    ClearPendingCommands();

    m_bControllerDirty = true;
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

void AssetBrowserViewController::ApplyProjectionModifiers()
{
    for (const FProjectionModifierEntry& entry : m_ProjectionModifiers)
    {
        if (entry.modifier)
            entry.modifier(m_View);
    }
}

void AssetBrowserViewController::ApplyPendingSettings()
{
    if (m_PendingCurrentPath)
    {
        m_Settings.currentPath = *m_PendingCurrentPath;
        m_bControllerDirty = true;
    }

    if (m_PendingRootPath)
    {
        m_Settings.rootPath = *m_PendingRootPath;
        m_bControllerDirty = true;
    }

    if (m_PendingIncludeRootNode)
    {
        m_Settings.bIncludeRootNode = *m_PendingIncludeRootNode;
        m_bControllerDirty = true;
    }

    if (m_PendingSearchFilter)
    {
        m_Settings.searchFilter = std::move(*m_PendingSearchFilter);
        m_bControllerDirty = true;
    }

    if (m_PendingShowFolders)
    {
        m_Settings.bShowFolders = *m_PendingShowFolders;
        m_bControllerDirty = true;
    }

    if (m_PendingShowAssets)
    {
        m_Settings.bShowAssets = *m_PendingShowAssets;
        m_bControllerDirty = true;
    }

    if (m_PendingProjectionMode)
    {
        m_Settings.projectionMode = *m_PendingProjectionMode;
        m_bControllerDirty = true;
    }
}

void AssetBrowserViewController::ApplyPendingExpansion(const AssetBrowserService& service)
{
    if (m_bCollapseAll)
    {
        if (!m_ExpandedNodes.empty())
        {
            m_ExpandedNodes.clear();
            m_bControllerDirty = true;
        }
    }

    if (m_bExpandAll)
    {
        bool bChanged = false;

        for (auto& [id, node] : m_View.nodeCache)
        {
            if (node.type == EAssetBrowserNodeType::Folder)
            {
                m_ExpandedNodes.insert(id);
                bChanged = true;
            }
        }

        if (bChanged)
            m_bControllerDirty = true;
    }

    for (AssetBrowserNodeID id : m_PendingExpand)
    {
        if (m_ExpandedNodes.insert(id).second)
        {
            m_bControllerDirty = true;
        }
    }

    for (AssetBrowserNodeID id : m_PendingCollapse)
    {
        if (m_ExpandedNodes.erase(id) > 0)
        {
            m_bControllerDirty = true;
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
