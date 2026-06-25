// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <span>

#include "EditorCore/IEditorPanel.h"

class IEditorPanel;
class EditorHost;
class EditorRuntime;
class EditorLayoutModel;
class AssetCacheService;

/**
 * Renderer interface for the Editor UI layer.
 *
 * Owns "chrome" drawing (menu/toolbar/dockspace) and draws the panel list provided
 * by the panel tracker. Backend-specific renderers (ImGui, HTML, etc.) implement this.
 */
class IEditorRenderer
{
public:
    virtual ~IEditorRenderer() = default;

    // Initialize with editor services. Typically called once on editor startup.
    virtual void Initialize() = 0;

    // Called once per frame between UIBackend BeginFrame/EndFrame
    virtual void RenderChrome(float deltaTime) = 0; ///< menu + toolbar + dockspace
    virtual void RenderPanels(std::span<IEditorPanel* const> panels) = 0; ///< draws panels
    virtual void RenderDialogs() = 0; ///< draw dialogs
};