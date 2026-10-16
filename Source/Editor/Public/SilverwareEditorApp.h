// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <cstdint>

#include "Core/IEditorBridge.h"
#include "Core/Memory/SmartPointers.h"

namespace Silverware
{
    class ContainerWidget;
    class TextWidget;

    namespace Backends::RmlUi
    {
        class RmlUiRuntime;
    }

    class IRenderingContext;
}

class EditorHost;
class EditorRuntime;
class IPlatformWindow;
class ProjectContext;

class SilverwareEditorApp final : public IEditorBridge
{
private:
    IPlatformWindow* m_Window = nullptr;

    TUniquePtr<Silverware::IRenderingContext> m_RenderingContext;
    TUniquePtr<Silverware::Backends::RmlUi::RmlUiRuntime> m_RmlUiRuntime;
    TUniquePtr<Silverware::ContainerWidget> m_RootWidget;

    TUniquePtr<EditorRuntime> m_EditorRuntime;
    TUniquePtr<EditorHost> m_EditorHost;

    Silverware::TextWidget* m_TestTextWidget = nullptr;
    Silverware::TextWidget* m_TestDynamicTextWidget = nullptr;

    float m_SyncTestElapsed = 0.0f;
    std::uint8_t m_SyncTestStage = 0;

public:
    SilverwareEditorApp();
    ~SilverwareEditorApp();

    void Shutdown();

    void OnProjectInitialized(IPlatformWindow* window, ProjectContext& projectCtx) override;
    void OnSceneLoaded(const std::string& sceneName) override;
    void OnRenderOverlay(float deltaTime) override;
    void OnTick(float deltaTime) override;
};