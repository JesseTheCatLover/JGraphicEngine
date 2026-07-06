//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Panels/Controllers/AssetBrowserController.h"

#include <iostream>

#include "EditorRuntime.h"
#include "EditorCore/EditorHost.h"
#include "Assets/FAssetRecord.h"
#include "EditorCore/Services/AssetCacheService.h"
#include "Panels/Controllers/Inputs/FAssetBrowserPanelInput.h"
#include "Panels/Controllers/Outputs/FAssetBrowserOutput.h"
#include "Utilities/UPath.h"

AssetBrowserController::AssetBrowserController(PanelID id, EditorHost& host, EditorRuntime& runtime)
    : m_PanelID(id)
      , m_Host(host)
      , m_Runtime(runtime)
, m_TreeViewController(
        FAssetBrowserViewSettings{
            .selectionPolicy = EAssetBrowserSelectionPolicy::LocalSelection,
            .currentPath = "/Project",
            .rootPath = "/Project",
            .bShowFolders = true,
            .bShowAssets = false,
            .bIncludeRootNode = false,
            .projectionMode = EAssetBrowserProjectionMode::Tree,
        })

    , m_ContentViewController(
        FAssetBrowserViewSettings{
            .selectionPolicy = EAssetBrowserSelectionPolicy::SharedGlobalSelection,
            .currentPath = "/Project",
            .rootPath = "/Project",
            .bShowFolders = true,
            .bShowAssets = true,
            .projectionMode = EAssetBrowserProjectionMode::Flat,
        })
{
    m_AssetsMutatedHandle = m_Host.GetService<AssetBrowserService>().OnAssetsMutated().Add([this](const FAssetOpResult&)
    {
        m_TreeViewController.RequestRefresh();
        m_ContentViewController.RequestRefresh();
    });

    auto& cache = m_Host.GetService<AssetCacheService>();

    m_FolderIcon = cache.GetTexture("AssetBrowser/FolderIcon");
    m_FolderEmptyIcon = cache.GetTexture("AssetBrowser/FolderEmptyIcon");
    m_AssetIcon = cache.GetTexture("AssetBrowser/AssetIcon");
}

AssetBrowserController::~AssetBrowserController()
{
}

void AssetBrowserController::Update(float /*deltaTime*/, const FAssetBrowserPanelInput &input, FAssetBrowserOutput &out)
{
    // Always reset out
    out = {};

    AssetBrowserService& service = m_Host.GetService<AssetBrowserService>();

    ProcessNavigation(input);
    ProcessSearch(input);
    ProcessExpansion(input);
    ProcessSelection(input);
    ProcessOpen(input);
    ProcessMutations(input, service);

    m_TreeViewController.Update(service);
    m_ContentViewController.Update(service);

    out.treeView = m_TreeViewController.GetProjection();
    out.contentView = m_ContentViewController.GetProjection();

    out.currentContentNavigationPath = m_ContentViewController.GetCurrentNavigationPath();
    out.currentTreeNavigationPath = m_TreeViewController.GetCurrentNavigationPath();

    out.bCanNavigateBack = !m_PreviousHistory.empty();
    out.bCanNavigateForward = !m_ForwardHistory.empty();

    out.currentTreeSearch = m_TreeViewController.GetSearchFilter();

    out.currentContentSearch = m_ContentViewController.GetSearchFilter();


    for (AssetBrowserNodeID id : out.treeView.viewNodeIDs)
    {
        const FAssetBrowserNode* node = service.GetNode(out.treeView, id);

        if (!node)
            continue;

        if (m_TreeViewController.IsSelected(service, node->nodeID))
        {
            out.selectedTreeNodes.insert(id);
        }
    }

    for (AssetBrowserNodeID id : out.contentView.viewNodeIDs)
    {
        const FAssetBrowserNode* node = service.GetNode(out.contentView, id);

        if (!node)
            continue;

        if (m_ContentViewController.IsSelected(service, node->nodeID))
        {
            out.selectedContentNodes.insert(id);
        }
    }

    out.icons.folder = m_Runtime.GetViewport().GetNativeTexture(m_FolderIcon);
    out.icons.folderEmpty = m_Runtime.GetViewport().GetNativeTexture(m_FolderEmptyIcon);
    out.icons.asset = m_Runtime.GetViewport().GetNativeTexture(m_AssetIcon);

    out.bValid = true;
}

void AssetBrowserController::OnPanelDestroyed()
{
    if (m_AssetsMutatedHandle.IsValid())
    {
        m_Host.GetService<AssetBrowserService>().OnAssetsMutated().Remove(m_AssetsMutatedHandle);
        m_AssetsMutatedHandle = {};
    }

    m_TreeViewController.Clear();
    m_ContentViewController.Clear();
}

void AssetBrowserController::NavigateTo(const std::string& path)
{
    const std::string current =
        m_ContentViewController.GetCurrentNavigationPath();

    if (current == path)
        return;

    if (!m_bInternalNavigation)
    {
        m_PreviousHistory.push_back(current);
        m_ForwardHistory.clear();

        ClampHistory(m_PreviousHistory);
        ClampHistory(m_ForwardHistory);
    }

    m_ContentViewController.RequestNavigateTo(path);
}

void AssetBrowserController::NavigateBack()
{
    if (m_PreviousHistory.empty())
        return;

    const std::string current = m_ContentViewController.GetCurrentNavigationPath();

    m_ForwardHistory.push_back(current);

    const std::string previous =m_PreviousHistory.back();
    m_PreviousHistory.pop_back();

    m_bInternalNavigation = true;
    m_ContentViewController.RequestNavigateTo(previous);
    m_bInternalNavigation = false;

    ClampHistory(m_PreviousHistory);
    ClampHistory(m_ForwardHistory);
}

void AssetBrowserController::NavigateForward()
{
    if (m_ForwardHistory.empty())
        return;

    const std::string current =
        m_ContentViewController.GetCurrentNavigationPath();

    m_PreviousHistory.push_back(current);

    const std::string next =
        m_ForwardHistory.back();
    m_ForwardHistory.pop_back();

    m_bInternalNavigation = true;
    m_ContentViewController.RequestNavigateTo(next);
    m_bInternalNavigation = false;

    ClampHistory(m_PreviousHistory);
    ClampHistory(m_ForwardHistory);
}

void AssetBrowserController::ProcessNavigation(const FAssetBrowserPanelInput &input)
{
    if (input.bNavigateHome)
    {
        NavigateTo("/Project");
    }

    if (input.bNavigateUp)
    {
        std::string current =
            m_ContentViewController.GetCurrentNavigationPath();

        const size_t slash = current.find_last_of('/');

        if (slash != std::string::npos)
        {
            current.resize(slash);

            if (current.empty())
                current = "/Project";

            NavigateTo(current);
        }
    }

    if (input.bNavigateToPath)
    {
        NavigateTo(input.navigateToPath);
    }

    if (input.bNavigatePrevious)
    {
        NavigateBack();
    }

    if (input.bNavigateNext)
    {
        NavigateForward();
    }
}

void AssetBrowserController::ProcessSearch(const FAssetBrowserPanelInput &input)
{
    if (input.bSetTreeSearch)
    {
        m_TreeViewController.RequestSetSearchFilter(input.treeSearch);
    }

    if (input.bSetContentSearch)
    {
        m_ContentViewController.RequestSetSearchFilter(input.contentSearch);
    }
}

void AssetBrowserController::ProcessExpansion(const FAssetBrowserPanelInput &input)
{
    for (const AssetBrowserNodeID id : input.expandNodes)
    {
        m_TreeViewController.RequestExpand(id);
    }

    for (const AssetBrowserNodeID id : input.collapseNodes)
    {
        m_TreeViewController.RequestCollapse(id);
    }
}

void AssetBrowserController::ProcessSelection(const FAssetBrowserPanelInput &input)
{
    if (input.bClearTreeSelection)
    {
        m_TreeViewController.RequestClearSelection();
    }

    for (const auto& selection : input.treeSelections)
    {
        m_TreeViewController.RequestSelectNode(
            selection.nodeID,
            selection.bToggle,
            selection.bRange);

        // Tree selection drives navigation (folder only)
        const auto& treeView = m_TreeViewController.GetProjection();

        auto it = treeView.nodeCache.find(selection.nodeID);
        if (it != treeView.nodeCache.end())
        {
            const FAssetBrowserNode& node = it->second;

            if (node.type == EAssetBrowserNodeType::Folder)
            {
                m_ContentViewController.RequestNavigateTo(node.virtualPath);
            }
        }
    }

    if (input.bClearContentSelection)
    {
        m_ContentViewController.RequestClearSelection();
    }

    for (const auto& selection : input.contentSelections)
    {
        m_ContentViewController.RequestSelectNode(
            selection.nodeID,
            selection.bToggle,
            selection.bRange);
    }
}

void AssetBrowserController::ProcessOpen(const FAssetBrowserPanelInput &input)
{
    if (input.bOpenNode)
    {
        const auto& view =
            m_ContentViewController.GetProjection();

        auto it = view.nodeCache.find(input.openNodeID);

        if (it != view.nodeCache.end())
        {
            const FAssetBrowserNode& node =
                it->second;

            if (node.type ==
                EAssetBrowserNodeType::Folder)
            {
                NavigateTo(node.virtualPath);
            }
            else
            {
                // future:
                // OpenAsset(node.assetID);
            }
        }
    }
}

void AssetBrowserController::ProcessMutations(
    const FAssetBrowserPanelInput &input,
    AssetBrowserService& service)
{
    FAssetOpResult aggregated;

    const auto& view =
        m_ContentViewController.GetProjection();

    for (const auto& mutation : input.mutations)
    {
        FAssetOpResult result;

        // Create Folder (no node needed)
        if (mutation.type == FAssetBrowserPanelInput::EMutationType::CreateFolder)
        {
            result = service.CreateFolder(mutation.destinationPath);

            AssetBrowserService::MergeOpResult(aggregated, result);

            continue;
        }

        const FAssetBrowserNode* node = service.GetNode(view, mutation.nodeID);

        if (!node)
            continue;

        if (node->type == EAssetBrowserNodeType::Folder)
        {
            switch (mutation.type)
            {
                case FAssetBrowserPanelInput::EMutationType::Rename:
                    result = service.RenameFolder(node->virtualPath, mutation.destinationPath);
                    break;

                case FAssetBrowserPanelInput::EMutationType::Delete:
                    result = service.DeleteFolder(node->virtualPath);
                    break;

                case FAssetBrowserPanelInput::EMutationType::Duplicate:
                    // mutationResult = service.CopyFolder(node->virtualPath, mutation.destinationPath);
                    // TODO: Implement folder copying through AssetManager -> EditorFileAPI -> AssetBrowserService
                    break;

                case FAssetBrowserPanelInput::EMutationType::Move:
                    result = service.MoveFolder(node->virtualPath, mutation.destinationPath);
                    break;

                default:
                    break;
            }
        }
        else
        {
            switch (mutation.type)
            {
                case FAssetBrowserPanelInput::EMutationType::Rename:
                    result = service.RenameAsset(node->virtualPath, mutation.destinationPath);
                    break;

                case FAssetBrowserPanelInput::EMutationType::Delete:
                    result = service.DeleteAsset(node->virtualPath);
                    break;

                case FAssetBrowserPanelInput::EMutationType::Duplicate:
                    result = service.DuplicateAsset(node->virtualPath, mutation.destinationPath);
                    break;

                case FAssetBrowserPanelInput::EMutationType::Move:
                    result = service.MoveAsset(node->virtualPath, mutation.destinationPath);
                    break;

                default:
                    break;
            }
        }

        AssetBrowserService::MergeOpResult(aggregated, result);
    }

    // log aggregated result once
    for (const auto& error : aggregated.errors) // TODO : Later replace with a popup UI Error handling
    {
        std::cerr << "[AssetBrowserController] " << error << "\n";
    }

    for (const auto& warning : aggregated.warnings)
    {

        std::cerr << "[AssetBrowserController] " << warning << "\n";
    }
}

void AssetBrowserController::ClampHistory(std::vector<std::string> &history)
{
    if (history.size() <= kMaxHistorySize)
        return;

    history.erase(history.begin(), history.begin() + (history.size() - kMaxHistorySize));
}
