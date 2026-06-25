//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <cstdint>
#include <unordered_set>

#include "EditorCore/IEditorPanel.h"
#include "EditorCore/Services/AssetBrowser/FAssetBrowserNode.h"
#include "Panels/Controllers/Outputs/FAssetBrowserOutput.h"

struct FAssetBrowserViewProjection;
struct FAssetBrowserPanelInput;
struct FAssetBrowserNode;

class AssetBrowserPanel final : public IEditorPanel
{
public:
    AssetBrowserPanel() = default;

    [[nodiscard]] const char* GetName() const override { return "AssetBrowser###AssetBrowser"; }
    [[nodiscard]] const char* GetPanelKey() const override { return "AssetBrowser"; }
    [[nodiscard]] EPanelDockGroup GetDockGroup() const override { return EPanelDockGroup::Single; }

    void OnDestroy(EditorHost& host) override;
    void Draw(EditorHost& host) override;

private:
    // Split view sizes
    float m_LeftPaneWidth = 260.0f;
    float m_MinLeftPaneWidth = 180.0f;
    float m_MinRightPaneWidth = 240.0f;

    // Grid view
    float m_IconSize = 64.0f;
    float m_GridPadding = 12.0f;

    // Search buffer (kept across frames)
    char m_SearchBuf[256] = {};

    // Breadcrumbs section
    bool m_bEditingBreadcrumbs = false;
    std::string m_BreadcrumbOriginalPath;
    char m_BreadcrumbEditBuffer[512];

    // Optional toggles
    bool m_ShowFoldersInGrid = true;
    bool m_ShowAssetsInGrid = true;

    AssetBrowserNodeID m_ContextMenuNode = 0;
    bool m_bItemPopupsOpen = false;

private:
    void DrawTreeNode(AssetBrowserNodeID nodeID, const FAssetBrowserViewProjection& treeView,
        const std::unordered_set<AssetBrowserNodeID>& selectedNodes, FAssetBrowserPanelInput& input);

    void DrawFolderTile(const FAssetBrowserNode& node, bool bSelected, float cellW, FAssetBrowserPanelInput& input,
        const FAssetBrowserOutput& output);
    void DrawAssetTile(const FAssetBrowserNode& node, bool bSelected, float cellW, FAssetBrowserPanelInput& input,
        const FAssetBrowserOutput& output);

    void BeginBreadcrumbEditing(const std::string& currentPath);
    void EndBreadcrumbEditing();

    bool IsValidPath(const std::string& path, EditorHost& host);
};
