// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <string>
#include "IEditorRenderer.h"
#include "imgui.h"

class ImGuiRenderer final : public IEditorRenderer
{
public:
    void Initialize(EditorHost& host, EditorRuntime& runtime, EditorLayoutModel& layout, EditorAssetCache& cache) override;

    void RenderChrome(float deltaTime) override;
    void RenderPanels(std::span<IEditorPanel* const> panels) override;

private:
    void DrawMainMenuBar();
    void DrawToolbar();
    void DrawDockspaceAndPanels(float deltaTime);

private:
    EditorHost* m_Host = nullptr;
    EditorRuntime* m_Runtime = nullptr;
    EditorLayoutModel* m_Layout = nullptr;
    EditorAssetCache* m_Cache = nullptr;

    ImGuiWindowClass m_ToolsDockClass{};
    ImGuiWindowClass m_ViewportDockClass{};
    ImGuiWindowClass m_ViewportHostDockClass{};
    bool m_BuiltDefaultLayout = false; // optional for DockBuilder defaults
    const ImGuiWindowClass* GetDockClassForPanel(const IEditorPanel& panel);
};
