//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorApp.h"

#include <GLFW/glfw3.h>
#include <Renderer/Backends/ImGuiBackend.h>
#include <iostream>

#include "EditorCore/EditorAssetCache.h"
#include "EditorCore/EditorHost.h"
#include "Renderer/EditorPanelTracker.h"
#include "EditorCore/Services/ShellCommandService.h"
#include "Renderer/ImGuiRenderer.h"
#include "EditorLayout/EditorLayoutModel.h"
#include "Rendering/IPlatformWindow.h"

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
   if (m_EditorHost)
   {
       m_EditorHost->Shutdown();
       m_EditorHost.reset();
   }

    if (m_EditorUIBackend)
    {
        m_EditorUIBackend->Shutdown();
        m_EditorUIBackend.reset();
    }

    m_Window = nullptr;
}

void EditorApp::OnProjectInitialized(IPlatformWindow* window)
{
    if (!window)
    {
        std::cerr << "[EditorApp]: window is null" << std::endl;
        return;
    }

    // Retrieve native window
    m_Window = window;

    // Grab native handle from the surface
    void* native = window->GetNativeHandle();

    GLFWwindow* nativeWindow = static_cast<GLFWwindow*>(native); // TODO: Should be backend agnostic in future by the BackendFactory
    if (!nativeWindow)
    {
        std::cerr << "[EditorApp]: native handle is not a valid GLFWwindow" << std::endl;
        return;
    }

    // Initialize ImGui backend
    m_EditorUIBackend = MakeUnique<ImGuiBackend>(nativeWindow);

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

    RegisterEditorShellCommands(*m_EditorHost, *m_LayoutModel);
    RegisterServicesShellCommands(*m_EditorHost);

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
    m_Renderer->RenderDialogs();
    EndFrame();
}

void EditorApp::OnTick(float deltaTime)
{
    if (m_EditorHost)
        m_EditorHost->Tick(deltaTime);
}

void EditorApp::RegisterEditorShellCommands(EditorHost& host, EditorLayoutModel& layout)
{
    auto& shell = host.GetService<ShellCommandService>();
    RegisterViewCommands(shell, layout);
    RegisterViewportCommands(shell, layout);
}

void EditorApp::RegisterViewCommands(ShellCommandService &shell, EditorLayoutModel &layout)
{
    shell.Register("Editor.View.ToggleSceneHierarchy", [&layout]() { layout.TogglePanelVisibility(EEditorPanelType::SceneHierarchy); });
    shell.Register("Editor.View.ToggleConsole",        [&layout]() { layout.TogglePanelVisibility(EEditorPanelType::Console); });
    shell.Register("Editor.View.ToggleAssetBrowser",   [&layout]() { layout.TogglePanelVisibility(EEditorPanelType::AssetBrowser); });
    shell.Register("Editor.View.ToggleInspector",      [&layout]() { layout.TogglePanelVisibility(EEditorPanelType::Inspector); });
}

void EditorApp::RegisterViewportCommands(ShellCommandService &shell, EditorLayoutModel &layout)
{
    shell.Register("Editor.Viewport.SetSingleView", [&layout]() { layout.SetViewportCount(1); });
    shell.Register("Editor.Viewport.SetDoubleView", [&layout]() { layout.SetViewportCount(2); });
    shell.Register("Editor.Viewport.SetTripleView", [&layout]() { layout.SetViewportCount(3); });
    shell.Register("Editor.Viewport.SetQuadView",   [&layout]() { layout.SetViewportCount(4); });
    shell.Register("Editor.Viewport.ToggleTabVisibility", [&layout]() { layout.ToggleShowViewportDocktabs(); });
}

void EditorApp::RegisterServicesShellCommands(EditorHost &host)
{
    host.RegisterShellCommandsForServices();
}
