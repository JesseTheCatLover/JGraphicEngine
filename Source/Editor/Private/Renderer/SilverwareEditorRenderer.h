// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Core/Memory/SmartPointers.h"
#include "Silverware/Memory/SmartPointers.h"

class IPlatformWindow;

namespace Silverware
{
    class IRenderingContext;
    class SilverwareUIContext;
    class ContainerWidget;
    class ImageWidget;
}

class EditorHost;
class EditorRuntime;
class SilverwareBackend;

class SilverwareEditorRenderer
{
public:
    SilverwareEditorRenderer();
    ~SilverwareEditorRenderer();

    SilverwareEditorRenderer(const SilverwareEditorRenderer&) = delete;
    SilverwareEditorRenderer& operator=(const SilverwareEditorRenderer&) = delete;

private:
    [[nodiscard]] bool BuildEditorLayout();

    EditorHost* m_EditorHost = nullptr;
    EditorRuntime* m_EditorRuntime = nullptr;
    SilverwareBackend* m_Backend = nullptr;

    Silverware::TUniquePtr<Silverware::ContainerWidget> m_RootWidget;

    TUniquePtr<Silverware::IRenderingContext> m_ViewportRenderingContext;
    TUniquePtr<Silverware::ContainerWidget> m_ViewportRootWidget;
    Silverware::SilverwareUIContext* m_ViewportUIContext = nullptr;
    Silverware::ContainerWidget* m_ViewportContextCard = nullptr;

    int m_ViewportContextWidth = 0;
    int m_ViewportContextHeight = 0;

    Silverware::ImageWidget* m_ViewportImage = nullptr;

    float m_ThemeSwitchElapsed = 0.0f;
    bool m_UseOrangeTheme = false;
    bool m_ThemeCleared = false;

    bool BuildViewportContextLayout();
    void UpdateViewportContextLayout(int width, int height);

public:
    bool Initialize(EditorHost& editorHost, EditorRuntime& editorRuntime,
     SilverwareBackend& backend, IPlatformWindow& window);

    void RenderViewportUIContext(IPlatformWindow& window);

    void Tick(float deltaTime);
    void Shutdown();
};