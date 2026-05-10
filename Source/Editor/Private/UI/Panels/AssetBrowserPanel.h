//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <cstdint>

#include "UI/IEditorPanels.h"

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

    // Selection / focus state
    std::string m_SelectedLeftPath;
    std::string m_SelectedAssetID;

    // Optional toggles
    bool m_ShowFoldersInGrid = true;
    bool m_ShowAssetsInGrid = true;
};
