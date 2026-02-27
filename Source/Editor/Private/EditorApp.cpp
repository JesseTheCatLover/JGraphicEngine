//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorApp.h"

#include <Core/CoreMinimal.h>
#include <Rendering/IPlatformSurface.h>
#include <GLFW/glfw3.h>
#include <Renderer/Backends/ImGuiBackend.h>
#include <iostream>

#include "Core/EditorAssetCache.h"
#include "Core/EditorHost.h"
#include "Core/EditorPanelTracker.h"
#include "Core/Services/ShellCommandService.h"
#include "Renderer/ImGuiRenderer.h"
#include "Layout/EditorLayoutModel.h"

EditorApp::EditorApp()
{
}

EditorApp::~EditorApp() = default;

void EditorApp::BeginFrame()
{
    m_EditorUIBackend->BeginFrame();
}

void EditorApp::EndFrame()
{
    if (m_EditorUIBackend)
        m_EditorUIBackend->EndFrame();
}

void EditorApp::Shutdown()
{
    // Destroy core/context
    m_EditorHost.reset();

    if (m_EditorUIBackend)
    {
        m_EditorUIBackend->Shutdown();
        m_EditorUIBackend.reset();
    }

    m_Window = nullptr;
}

void EditorApp::OnEngineInitialized(IPlatformSurface* surface)
{
    if (!surface)
    {
        std::cerr << "[EditorApp]: surface is null" << std::endl;
        return;
    }

    // Grab native handle from the surface
    void* native = surface->GetNativeHandle();

    GLFWwindow* window = static_cast<GLFWwindow*>(native);
    if (!window)
    {
        std::cerr << "[EditorApp]: native handle is not a valid GLFWwindow" << std::endl;
        return;
    }

    // Retrieve native GLFW window
    m_Window = window;
    if (!m_Window)
    {
        std::cerr << "[EditorApp]: Native surface handle is not a GLFWwindow " << std::endl;
        return;
    }


    // Initialize ImGui backend
    m_EditorUIBackend = MakeUnique<ImGuiBackend>(window);

    // EditorRuntime (safe bridge API to Engine)
    m_EditorRuntime = TUniquePtr<EditorRuntime>(new EditorRuntime());

    // Create EditorCore to drive context & commands
    m_EditorHost = TUniquePtr<EditorHost>(new EditorHost(*m_EditorRuntime));

    m_LayoutModel = MakeUnique<EditorLayoutModel>();
    m_LayoutModel->ResetToDefaults();

    m_PanelTracker = MakeUnique<EditorPanelTracker>();
    m_PanelTracker->Initialize(*m_EditorHost);
    m_PanelTracker->ApplyLayout(*m_EditorHost, *m_LayoutModel); // initial sync

    m_EditorCache = MakeUnique<EditorAssetCache>();
    m_EditorCache->PreloadAll(*m_EditorRuntime);

    RegisterDefaultShellCommands();

    m_Renderer = MakeUnique<ImGuiRenderer>();
    m_Renderer->Initialize(*m_EditorHost, *m_EditorRuntime, *m_LayoutModel, *m_EditorCache);
}

void EditorApp::OnSceneLoaded(const std::string &sceneName)
{
}

void EditorApp::OnRenderOverlay(float deltaTime)
{
    BeginFrame();
    m_Renderer->RenderChrome(deltaTime); // menu clicks happen here
    m_PanelTracker->ApplyLayout(*m_EditorHost, *m_LayoutModel); // reacts same frame
    m_Renderer->RenderPanels(m_PanelTracker->GetDrawPanels()); // draws updated panels
    EndFrame();
}

void EditorApp::OnTick(float deltaTime)
{
    if (m_EditorHost)
        m_EditorHost->Tick(deltaTime);
}

void EditorApp::RegisterDefaultShellCommands() // TODO: Search and check if we should centralize the registration of shell command or each field should register itself
{
    auto& shell = m_EditorHost->GetService<ShellCommandService>();

    shell.Register("Editor.View.ToggleSceneHierarchy", [this]()
    {
        m_LayoutModel->TogglePanelVisibility(EEditorPanelType::SceneHierarchy);
    });

    shell.Register("Editor.View.ToggleConsole", [this]()
    {
        m_LayoutModel->TogglePanelVisibility(EEditorPanelType::Console);
    });

    shell.Register("Editor.View.ToggleAssetBrowser", [this]()
    {
        m_LayoutModel->TogglePanelVisibility(EEditorPanelType::AssetBrowser);
    });

    shell.Register("Editor.View.ToggleInspector", [this]()
    {
        m_LayoutModel->TogglePanelVisibility(EEditorPanelType::Inspector);
    });

    shell.Register("Editor.Viewport.SetSingleView", [this]()
    {
        m_LayoutModel->SetViewportCount(1);
    });

    shell.Register("Editor.Viewport.SetDoubleView", [this]()
    {
        m_LayoutModel->SetViewportCount(2);
    });

    shell.Register("Editor.Viewport.SetTripleView", [this]()
    {
        m_LayoutModel->SetViewportCount(3);
    });

    shell.Register("Editor.Viewport.SetQuadView", [this]()
    {
        m_LayoutModel->SetViewportCount(4);
    });
}
