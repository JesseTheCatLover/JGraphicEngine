//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "UI/IEditorPanels.h"

class EditorCore;
class EditorContext;

class SceneViewportPanel : public IEditorPanel
{
public:
    SceneViewportPanel() = default;
    ~SceneViewportPanel() override = default;

    const char* GetName() const override { return "Viewport"; }

    void Draw(EditorContext& context, EditorCore& core) override;

private:
    // Last known viewport size (for future use: camera aspect, RT resize, etc.)
    int m_Width = 0.0f;
    int m_Height = 0.0f; // TODO: Check if we should calculate by float or int

    // Focus/hover state if you want to drive editor camera later
    bool m_IsFocused = false;
    bool m_IsHovered = false;
};