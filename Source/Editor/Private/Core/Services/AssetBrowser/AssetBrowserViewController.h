//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <unordered_set>
#include <optional>
#include <functional>
#include <vector>

#include "FAssetBrowserViewProjection.h"
#include "AssetBrowserService.h"
#include "FAssetBrowserViewSettings.h"
#include "Core/Memory/SmartPointers.h"
#include "Core/Delegates/FDelegateHandle.h"
#include "Utilities/UDynamicID.h"

// Forward decl
template <typename T> class TSelectionModel;

/**
 * Controller responsible for managing the state and projection of a
 * single asset browser view.
 *
 * The controller owns view-local concerns such as navigation,
 * filtering, expansion state, projection mode, and selection policy.
 * UI interactions are accumulated as pending commands and applied
 * during Update().
 *
 * AssetBrowserService remains the source of truth for asset hierarchy
 * data. The controller requests projection rebuilds from the service
 * and maintains the resulting FAssetBrowserViewProjection used by UI
 * panels.
 *
 * Projection modifiers may be registered to perform additional
 * view-specific transformations without mutating the underlying asset
 * browser model.
 */
class AssetBrowserViewController
{
public:
    using FProjectionModifier = std::function<void(FAssetBrowserViewProjection&)>;

    struct FProjectionModifierEntry
    {
        FDelegateHandle handle;
        FProjectionModifier modifier;
    };

private:
    FAssetBrowserViewSettings m_Settings;
    FAssetBrowserViewProjection m_View;

    bool m_bControllerDirty = false;

    UDynamicID m_ModifierIDGenerator;
    std::vector<FProjectionModifierEntry> m_ProjectionModifiers;

    std::unordered_set<AssetBrowserNodeID> m_ExpandedNodes;

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
    explicit AssetBrowserViewController(FAssetBrowserViewSettings initialSettings);
    ~AssetBrowserViewController();

    [[nodiscard]] const FAssetBrowserViewSettings& GetSettings() const { return m_Settings; }
    [[nodiscard]] const FAssetBrowserViewProjection& GetProjection() const {return m_View; }

    FDelegateHandle AddProjectionModifier(FProjectionModifier modifier);
    bool RemoveProjectionModifier(FDelegateHandle modifierHandle);
    void ClearAllProjectionModifiers();

    // Navigation
    [[nodiscard]] const std::string& GetCurrentNavigationPath();
    void RequestNavigateTo(const std::string& path);
    void RequestSetRoot(const std::string& path);

    void RequestIncludeRootNode(bool value);

    // Expansion
    void RequestExpandRoot(AssetBrowserService& service);
    void RequestCollapseRoot(AssetBrowserService& service);

    void RequestExpand(AssetBrowserNodeID id);
    void RequestCollapse(AssetBrowserNodeID id);

    void RequestExpandAll();
    void RequestCollapseAll();

    [[nodiscard]] bool IsFolderExpanded(AssetBrowserNodeID id) const;

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

    void Update(AssetBrowserService& service);

    void Clear();

private:
    [[nodiscard]] TSelectionModel<std::string>& GetSelectionModel(AssetBrowserService& service);
    [[nodiscard]] const TSelectionModel<std::string>& GetSelectionModel(const AssetBrowserService& service) const;

    void ApplyProjectionModifiers();

    void ApplyPendingSettings();

    void ApplyPendingExpansion(const AssetBrowserService& service);

    void ApplyPendingSelection(AssetBrowserService& service, const FAssetBrowserViewProjection& view);

    void ClearPendingCommands();
};