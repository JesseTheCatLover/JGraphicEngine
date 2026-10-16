// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareEditorApp.h"

#include "EditorRuntime.h"
#include "EditorCore/EditorHost.h"
#include "Core/Project/ProjectContext.h"
#include "Rendering/IPlatformWindow.h"

#include <iostream>

SilverwareEditorApp::SilverwareEditorApp()
{
}

SilverwareEditorApp::~SilverwareEditorApp()
{
    Shutdown();
}

void SilverwareEditorApp::Shutdown()
{
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
    // Silverware rendering will be integrated here.
}