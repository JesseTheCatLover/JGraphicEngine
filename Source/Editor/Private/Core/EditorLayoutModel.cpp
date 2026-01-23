//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorLayoutModel.h"

static int ClampViewportCount(int v)
{
    if (v < 1) return 1;
    if (v > 4) return 4;
    return v;
}

void EditorLayoutModel::ResetToDefaults()
{
    m_ViewportCount = 1;
    m_ToolVisible.fill(false);
    m_ToolVisible[(size_t)EEditorPanelType::SceneHierarchy] = true;

    m_ViewportCountDirty = true;
    m_ToolDirty.fill(true);
}

void EditorLayoutModel::SetViewportCount(int count)
{
    count = ClampViewportCount(count);
    if (m_ViewportCount == count) return;
    m_ViewportCount = count;
    m_ViewportCountDirty = true;
}

bool EditorLayoutModel::ConsumeViewportCountChanged()
{
    bool bWas = m_ViewportCountDirty;
    m_ViewportCountDirty = false;
    return bWas;
}

bool EditorLayoutModel::IsPanelVisible(EEditorPanelType id) const
{
    return m_ToolVisible[(size_t)id];
}

void EditorLayoutModel::SetPanelVisible(EEditorPanelType id, bool bVisible)
{
    auto& v = m_ToolVisible[(size_t)id];
    if (v == bVisible) return;
    v = bVisible;
    m_ToolDirty[(size_t)id] = true;
}

void EditorLayoutModel::TogglePanelVisibility(EEditorPanelType id)
{
    SetPanelVisible(id, !IsPanelVisible(id));
}

bool EditorLayoutModel::ConsumePanelVisibilityChanged(EEditorPanelType id)
{
    bool bWas = m_ToolDirty[(size_t)id];
    m_ToolDirty[(size_t)id] = false;
    return bWas;
}

// ---- Descriptors ----

std::string_view EditorLayoutModel::ViewportKey(int index)
{
    // string literals so views stay valid forever
    static constexpr std::string_view keys[4] = {
        "Viewport0", "Viewport1", "Viewport2", "Viewport3"
    };
    if (index < 0 || index > 3) return "Viewport?";
    return keys[index];
}

std::string_view EditorLayoutModel::ViewportName(int index)
{
    static constexpr std::string_view names[4] = {
        "Viewport 0", "Viewport 1", "Viewport 2", "Viewport 3"
    };
    if (index < 0 || index > 3) return "Viewport (?)";
    return names[index];
}

FEditorPanelDesc EditorLayoutModel::GetViewportDesc(int index) const
{
    FEditorPanelDesc d;
    d.kind = EEditorPanelKind::MultiPanel;
    d.key = ViewportKey(index);
    d.name = ViewportName(index);
    return d;
}

FEditorPanelDesc EditorLayoutModel::GetSinglePanelDesc(EEditorPanelType id) const
{
    FEditorPanelDesc d;
    d.kind = EEditorPanelKind::SinglePanel;

    switch (id)
    {
        case EEditorPanelType::SceneHierarchy:
            d.key = "SceneHierarchy";
            d.name = "Scene Hierarchy";
            break;
        case EEditorPanelType::Inspector:
            d.key = "Inspector";
            d.name = "Inspector";
            break;
        case EEditorPanelType::AssetBrowser:
            d.key = "AssetBrowser";
            d.name = "Asset Browser";
            break;
        case EEditorPanelType::Console:
            d.key = "Console";
            d.name = "Console";
            break;
        default:
            d.key = "Unknown";
            d.name = "Unknown";
            break;
    }
    return d;
}
