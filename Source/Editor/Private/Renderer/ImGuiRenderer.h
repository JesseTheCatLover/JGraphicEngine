//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <span>

#include "UI/IEditorPanels.h"

class EditorHost;
class EditorRuntime;
class EditorLayoutModel;
class EditorAssetCache;

class ImGuiRenderer
{
public:
    void Initialize(EditorHost& host, EditorRuntime& runtime, EditorLayoutModel& layout, EditorAssetCache& cache);

    // Call once per frame between BeginFrame/EndFrame:

    void RenderChrome(float deltaTime); ///< menu + toolbar + dockspace
    void RenderPanels(std::span<IEditorPanel* const> panels); ///< draws panels

private:
    void DrawMainMenuBar();
    void DrawToolbar();
    void DrawDockspaceAndPanels(float dt);

private:
    EditorHost* m_Host = nullptr;
    EditorRuntime* m_Runtime = nullptr;
    EditorLayoutModel* m_Layout = nullptr;
    EditorAssetCache* m_Cache = nullptr;

    bool m_ShowViewportDockTabs = true; ///< Toggle Tab Visibility For Viewport
};
