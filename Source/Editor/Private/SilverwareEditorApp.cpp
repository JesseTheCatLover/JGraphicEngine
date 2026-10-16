// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareEditorApp.h"

#include "EditorRuntime.h"
#include "EditorCore/EditorHost.h"
#include "Core/Project/ProjectContext.h"
#include "Silverware/Backends/RmlUi/RmlUiRuntime.h"
#include "Silverware/Backend/IRenderingContext.h"
#include "Rendering/IPlatformWindow.h"

#include <iostream>

#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

SilverwareEditorApp::SilverwareEditorApp()
{
}

SilverwareEditorApp::~SilverwareEditorApp()
{
    Shutdown();
}

void SilverwareEditorApp::Shutdown()
{
    if (m_RmlUiRuntime)
    {
        m_RmlUiRuntime->Shutdown();
        m_RmlUiRuntime.reset();
    }

    if (m_EditorHost)
    {
        m_EditorHost->Shutdown();
        m_EditorHost.reset();
    }

    m_RenderingContext.reset();
    m_Window = nullptr;
    m_EditorRuntime.reset();
}

void SilverwareEditorApp::OnProjectInitialized(IPlatformWindow* window, ProjectContext& projectCtx)
{
    if (!window)
    {
        std::cerr << "[SilverwareEditorApp]: Window is null.\n";
        return;
    }

    Shutdown();

    m_Window = window;
    m_EditorRuntime = TUniquePtr<EditorRuntime>(new EditorRuntime());
    m_EditorHost = TUniquePtr<EditorHost>(new EditorHost(*m_EditorRuntime));

    m_RenderingContext = m_EditorRuntime->CreateSilverwareRenderingContext();

    if (!m_RenderingContext)
    {
        std::cerr << "[SilverwareEditorApp]: Failed to create rendering context.\n";
        Shutdown();
        return;
    }

    int width = 0;
    int height = 0;
    window->GetFramebufferSize(width, height);

    const std::string fontPath = UPath::Join(
        UFileSystem::GetEngineRoot().string(),
        "Frameworks",
        "Silverware",
        "ThirdParty",
        "RmlUi",
        "Samples",
        "assets",
        "LatoLatin-Regular.ttf"
    );

    m_RmlUiRuntime = MakeUnique<Silverware::Backends::RmlUi::RmlUiRuntime>();

    if (!m_RmlUiRuntime->Initialize(
            *m_RenderingContext, width, height, fontPath))
    {
        std::cerr << "[SilverwareEditorApp]: Failed to initialize RmlUi.\n";
        Shutdown();
        return;
    }

    // Silverware backend and UI initialization will be added here.
    (void)projectCtx;
}

void SilverwareEditorApp::OnSceneLoaded(const std::string&)
{
}

void SilverwareEditorApp::OnTick(float deltaTime)
{
    if (m_EditorHost)
        m_EditorHost->Tick(deltaTime);
}

void SilverwareEditorApp::OnRenderOverlay(float)
{
    if (!m_Window || !m_RmlUiRuntime)
        return;

    int width = 0;
    int height = 0;
    m_Window->GetFramebufferSize(width, height);

    if (width <= 0 || height <= 0)
        return;

    m_RmlUiRuntime->Resize(width, height);
    m_RmlUiRuntime->Update();
    m_RmlUiRuntime->Render();
}