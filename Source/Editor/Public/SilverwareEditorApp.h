// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>

#include "Core/IEditorBridge.h"
#include "Core/Memory/SmartPointers.h"

class SilverwareEditorRenderer;
class SilverwareBackend;
class EditorHost;
class EditorRuntime;
class IPlatformWindow;
class ProjectContext;

class SilverwareEditorApp final : public IEditorBridge
{
private:
    IPlatformWindow* m_Window = nullptr;

    TUniquePtr<SilverwareBackend> m_SilverwareBackend;
    TUniquePtr<SilverwareEditorRenderer> m_SilverwareRenderer;

    TUniquePtr<EditorRuntime> m_EditorRuntime;
    TUniquePtr<EditorHost> m_EditorHost;

public:
    SilverwareEditorApp();
    ~SilverwareEditorApp();

    void Shutdown();

    void OnProjectInitialized(IPlatformWindow* window, ProjectContext& projectCtx) override;

    void OnSceneLoaded(const std::string& sceneName) override;
    void OnRenderOverlay(float deltaTime) override;
    void OnTick(float deltaTime) override;
};