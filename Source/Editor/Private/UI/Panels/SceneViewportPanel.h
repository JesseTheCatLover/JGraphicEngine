//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#include "UI/IEditorPanels.h"

class EditorCore;
class EditorContext;

class SceneViewportPanel : public IEditorPanel
{
public:
    explicit SceneViewportPanel(int index)
    {
        // Visible title: "Viewport 1", internal ID: "Viewport_1"
        char buf[64];
        snprintf(buf, sizeof(buf), "Viewport %d###Viewport_%d", index, index);
        m_WindowName = buf;
    }
    ~SceneViewportPanel() override = default;

    const char* GetName() const override { return m_WindowName.c_str(); }

    void Draw(EditorContext& context, EditorCore& core) override;

    void OnCreate(EditorContext &context, EditorCore &core) override;

    void OnDestroy(EditorContext &context, EditorCore &core) override;

private:
    std::string m_WindowName;
    // Last known viewport size (for future use: camera aspect, RT resize, etc.)
    float m_Width = 0.f;
    float m_Height = 0.f;

    // Focus/hover state if you want to drive editor camera later
    bool m_IsFocused = false;
    bool m_IsHovered = false;
    bool m_HasMouseCapture = false;
};