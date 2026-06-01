//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <unordered_set>
#include <optional>
#include <vector>

#include "FAssetBrowserViewProjection.h"
#include "AssetBrowserService.h"
#include "Core/Memory/SmartPointers.h"

// Forward decl
template <typename T> class TSelectionModel;

enum class EAssetBrowserSelectionPolicy
{
    SharedGlobalSelection, // uses SelectionService asset-path selection
    LocalSelection         // controller owns its own TSelectionModel<std::string>
};

enum class EAssetBrowserProjectionMode : uint8_t
{
    Flat,   // only immediate children of currentPath
    Tree    // recursive tree from a rootPath
};

struct FAssetBrowserViewSettings
{
    // Selection policy (per-controller)
    EAssetBrowserSelectionPolicy selectionPolicy = EAssetBrowserSelectionPolicy::SharedGlobalSelection;

    std::string currentPath = "/Project";
    std::string rootPath = "/Project";

    std::string searchFilter;

    bool bShowFolders = true;
    bool bShowAssets = true;

    bool bIncludeRootNode = true;

    EAssetBrowserProjectionMode projectionMode = EAssetBrowserProjectionMode::Flat;

    std::unordered_set<AssetBrowserNodeID> expandedFolderNodes;
};

class AssetBrowserViewController
{
private:
    FAssetBrowserViewSettings m_Settings;
    bool m_bProjectionDirty = false;

    // Only used if selectionPolicy == LocalSelection
    TUniquePtr<TSelectionModel<std::string>> m_LocalSelectionModel;

    std::optional<std::string> m_PendingCurrentPath;
    std::optional<std::string> m_PendingRootPath;

    std::optional<bool> m_PendingIncludeRootNode;

    std::optional<std::string> m_PendingSearchFilter;

    std::optional<bool> m_PendingShowFolders;
    std::optional<bool> m_PendingShowAssets;

    std::optional<EAssetBrowserProjectionMode> m_PendingProjectionMode;

    bool m_bExpandAll = false;
    bool m_bCollapseAll = false;

    std::unordered_set<AssetBrowserNodeID> m_PendingExpand;

    std::unordered_set<AssetBrowserNodeID> m_PendingCollapse;

    struct FSelectionRequest
    {
        AssetBrowserNodeID id;
        bool bToggle;
        bool bRange;
    };

    std::vector<FSelectionRequest> m_PendingSelections;

    bool m_bClearSelection = false;

public:
    // For compilation of unique pointer TClass
    AssetBrowserViewController();
    ~AssetBrowserViewController();
    AssetBrowserViewController(AssetBrowserViewController&&) noexcept;
    AssetBrowserViewController& operator=(AssetBrowserViewController&&) noexcept;
    AssetBrowserViewController(const AssetBrowserViewController&) = delete;
    AssetBrowserViewController& operator=(const AssetBrowserViewController&) = delete;

    [[nodiscard]] const FAssetBrowserViewSettings& GetSettings() const { return m_Settings; }

    // Navigation
    void RequestNavigateTo(const std::string& path);
    void RequestSetRoot(const std::string& path);

    void RequestIncludeRootNode(bool value);

    // Expansion
    void RequestExpand(AssetBrowserNodeID id);
    void RequestCollapse(AssetBrowserNodeID id);

    void RequestExpandAll();
    void RequestCollapseAll();

    // Filtering
    void RequestSetSearchFilter(std::string filter);

    // Visibility
    void RequestShowFolders(bool value);
    void RequestShowAssets(bool value);

    // Projection
    void RequestProjectionMode(EAssetBrowserProjectionMode mode);

    // Selection
    void RequestSelectNode(AssetBrowserNodeID id, bool bToggle = false, bool bRange = false);

    void RequestClearSelection();

    // Refresh
    void RequestRefresh();

    void Flush(AssetBrowserService& service, FAssetBrowserViewProjection& view);

    void Clear();

private:
    [[nodiscard]] TSelectionModel<std::string>& GetSelectionModel(AssetBrowserService& service);
    [[nodiscard]] const TSelectionModel<std::string>& GetSelectionModel(const AssetBrowserService& service) const;

    void ApplyPendingSettings();

    void ApplyPendingExpansion(const AssetBrowserService& service);

    void ApplyPendingSelection(AssetBrowserService& service, const FAssetBrowserViewProjection& view);

    void ClearPendingCommands();
};