// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <string>
#include "IEditorRenderer.h"
#include "imgui.h"

class EditTimelineService;

class ImGuiRenderer final : public IEditorRenderer
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;
    EditorLayoutModel& m_Layout;

    AssetCacheService& m_Cache;
    EditTimelineService& m_EditTimeLine;

    ImGuiWindowClass m_ToolsDockClass{};
    ImGuiWindowClass m_ViewportDockClass{};
    ImGuiWindowClass m_ViewportHostDockClass{};
    bool m_BuiltDefaultLayout = false; // optional for DockBuilder defaults
    const ImGuiWindowClass* GetDockClassForPanel(const IEditorPanel& panel);

public:
    ImGuiRenderer(EditorHost& host, EditorRuntime& runtime, EditorLayoutModel& layout);
    void Initialize() override;

    void RenderChrome(float deltaTime) override;
    void RenderPanels(std::span<IEditorPanel* const> panels) override;
    void RenderDialogs() override;

private:
    void DrawMainMenuBar();
    void DrawToolbar();
    void DrawDockspaceAndPanels(float deltaTime);
};
