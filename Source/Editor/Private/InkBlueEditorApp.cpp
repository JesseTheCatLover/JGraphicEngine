//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InkBlueEditorApp.h"

#include <GLFW/glfw3.h>
#include <Renderer/Backends/ImGuiBackend.h>
#include <iostream>

#include "Core/Project/ProjectContext.h"
#include "EditorCore/Services/AssetCacheService.h"
#include "EditorCore/EditorHost.h"
#include "Renderer/EditorPanelTracker.h"
#include "EditorCore/Services/ShellCommandService.h"
#include "Renderer/ImGuiRenderer.h"
#include "EditorLayout/EditorLayoutModel.h"
#include "Rendering/IPlatformWindow.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

InkBlueEditorApp::InkBlueEditorApp()
{
}

InkBlueEditorApp::~InkBlueEditorApp() = default;

void InkBlueEditorApp::BeginFrame()
{
    m_EditorUIBackend->BeginFrame();
}

void InkBlueEditorApp::EndFrame()
{
    if (m_EditorUIBackend)
        m_EditorUIBackend->EndFrame();
}

void InkBlueEditorApp::Shutdown()
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

void InkBlueEditorApp::OnProjectInitialized(IPlatformWindow* window, ProjectContext& projectCtx)
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

    EnsureDefaultEditorLayoutExists(projectCtx);

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

    RegisterEditorShellCommands(*m_EditorHost, *m_LayoutModel);
    RegisterServicesShellCommands(*m_EditorHost);

    m_Renderer = MakeUnique<ImGuiRenderer>(*m_EditorHost, *m_EditorRuntime, *m_LayoutModel);
    m_Renderer->Initialize();
}

void InkBlueEditorApp::OnSceneLoaded(const std::string &sceneName)
{
}

void InkBlueEditorApp::OnRenderOverlay(float deltaTime)
{
    BeginFrame();
    m_Renderer->RenderChrome(deltaTime); // menu clicks happen here
    m_PanelTracker->ApplyLayout(*m_EditorHost, *m_LayoutModel); // reacts same frame
    m_Renderer->RenderPanels(m_PanelTracker->GetDrawPanels()); // draws updated panels
    m_Renderer->RenderDialogs();
    EndFrame();
}

void InkBlueEditorApp::OnTick(float deltaTime)
{
    if (m_EditorHost)
        m_EditorHost->Tick(deltaTime);
}

void InkBlueEditorApp::EnsureDefaultEditorLayoutExists(ProjectContext& projectCtx)
{
    // 1. Resolve target project layout path
    const std::string& configRoot = projectCtx.GetProjectConfigRoot();
    const std::string projectLayoutDir = UPath::Join(configRoot, "Editor", "Layout", "ImGui");
    const std::string projectLayoutFile = UPath::Join(projectLayoutDir, "CurrentLayout.ini");

    // Early return if the project already has an active layout
    if (UFileSystem::FileExists(projectLayoutFile))
    {
        return;
    }

    // Ensure the project's Configs/Editor/Layout hierarchy exists
    if (!UFileSystem::CreateDirectory(projectLayoutDir))
    {
        std::cerr << "[EditorApp]: Failed to construct layout directory tree.\n";
        return;
    }

    // 2. Attempt to pull from the Engine's DefaultLayout.ini
    std::string engineRoot = UFileSystem::GetEngineRoot().string();
    std::string engineLayoutFile = UPath::Join(engineRoot, "Configs", "Editor", "Layout", "ImGui", "DefaultLayout.ini");

    if (UFileSystem::FileExists(engineLayoutFile))
    {
        // Copy the template directly to the project directory, bypassing memory load
        if (UFileSystem::CopyFile(engineLayoutFile, projectLayoutFile))
        {
            return; // Success!
        }
        std::cerr << "[EditorApp]: Failed to copy engine template. Falling back to hardcoded defaults.\n";
    }

    // 3. Hardcoded Fallback (Used if Engine configs are completely missing)
    std::string fallbackLayoutData =
        "[Window][Debug##Default]\n"
        "Pos=60,60\n"
        "Size=400,400\n"
        "Collapsed=0\n"
        "\n"
        "[Window][DockSpaceRoot]\n"
        "Pos=0,56\n"
        "Size=1512,861\n"
        "Collapsed=0\n"
        "\n"
        "[Window][ViewportDockHost]\n"
        "Pos=212,56\n"
        "Size=928,861\n"
        "Collapsed=0\n"
        "DockId=0x00000002\n"
        "ClassId=0xB1526A4D\n"
        "\n"
        "[Window][###SceneHierarchy]\n"
        "Pos=0,56\n"
        "Size=210,861\n"
        "Collapsed=0\n"
        "DockId=0x00000001,0\n"
        "ClassId=0xB1526A4D\n"
        "\n"
        "[Window][###Inspector]\n"
        "Pos=1142,56\n"
        "Size=370,861\n"
        "Collapsed=0\n"
        "DockId=0x00000004,0\n"
        "ClassId=0xB1526A4D\n"
        "\n"
        "[Window][Viewport 0##Viewport0]\n"
        "Pos=212,56\n"
        "Size=928,861\n"
        "Collapsed=0\n"
        "DockId=0x00000005,0\n"
        "ClassId=0x407E8E0F\n"
        "\n"
        "[Window][Viewport 2##Viewport2]\n"
        "Pos=678,56\n"
        "Size=462,861\n"
        "Collapsed=0\n"
        "DockId=0x00000009,0\n"
        "ClassId=0x407E8E0F\n"
        "\n"
        "[Window][Viewport 1##Viewport1]\n"
        "Pos=0,465\n"
        "Size=676,452\n"
        "Collapsed=0\n"
        "DockId=0x00000006,0\n"
        "ClassId=0x407E8E0F\n"
        "\n"
        "[Window][Viewport 3##Viewport3]\n"
        "Pos=1035,464\n"
        "Size=477,453\n"
        "Collapsed=0\n"
        "DockId=0x0000000A,0\n"
        "ClassId=0x407E8E0F\n"
        "\n"
        "[Window][###AssetBrowser]\n"
        "Pos=228,245\n"
        "Size=998,516\n"
        "Collapsed=0\n"
        "\n"
        "[Docking][Data]\n"
        "DockSpace     ID=0x4D4B13E2 Window=0x78E5D268 Pos=0,56 Size=1512,861 Split=X Selected=0xCBDCD176\n"
        "  DockNode    ID=0x00000003 Parent=0x4D4B13E2 SizeRef=1140,861 Split=X\n"
        "    DockNode  ID=0x00000001 Parent=0x00000003 SizeRef=210,861 Selected=0x58D44DEE\n"
        "    DockNode  ID=0x00000002 Parent=0x00000003 SizeRef=928,861 CentralNode=1 Selected=0xCBDCD176\n"
        "  DockNode    ID=0x00000004 Parent=0x4D4B13E2 SizeRef=370,861 Selected=0xCE855E27\n"
        "DockSpace     ID=0x82BC73AD Window=0x04DDEE09 Pos=212,56 Size=928,861 Split=X Selected=0xD724520A\n"
        "  DockNode    ID=0x00000007 Parent=0x82BC73AD SizeRef=464,861 Split=Y\n"
        "    DockNode  ID=0x00000005 Parent=0x00000007 SizeRef=982,407 CentralNode=1 Selected=0xD724520A\n"
        "    DockNode  ID=0x00000006 Parent=0x00000007 SizeRef=982,452 Selected=0x1D393058\n"
        "  DockNode    ID=0x00000008 Parent=0x82BC73AD SizeRef=462,861 Split=Y Selected=0x46F2E05F\n"
        "    DockNode  ID=0x00000009 Parent=0x00000008 SizeRef=498,406 Selected=0x46F2E05F\n"
        "    DockNode  ID=0x0000000A Parent=0x00000008 SizeRef=498,453 Selected=0x8CEF820D\n";

    if (!UFileSystem::WriteTextFile(projectLayoutFile, fallbackLayoutData))
    {
        std::cerr << "[EditorApp]: Failed to write fallback CurrentLayout.ini.\n";
    }
}

void InkBlueEditorApp::RegisterEditorShellCommands(EditorHost& host, EditorLayoutModel& layout)
{
    auto& shell = host.GetService<ShellCommandService>();
    RegisterViewCommands(shell, layout);
    RegisterViewportCommands(shell, layout);
}

void InkBlueEditorApp::RegisterViewCommands(ShellCommandService &shell, EditorLayoutModel &layout)
{
    shell.Register("Editor.View.ToggleSceneHierarchy", [&layout]() { layout.TogglePanelVisibility(EEditorPanelType::SceneHierarchy); });
    shell.Register("Editor.View.ToggleConsole",        [&layout]() { layout.TogglePanelVisibility(EEditorPanelType::Console); });
    shell.Register("Editor.View.ToggleAssetBrowser",   [&layout]() { layout.TogglePanelVisibility(EEditorPanelType::AssetBrowser); });
    shell.Register("Editor.View.ToggleInspector",      [&layout]() { layout.TogglePanelVisibility(EEditorPanelType::Inspector); });
}

void InkBlueEditorApp::RegisterViewportCommands(ShellCommandService &shell, EditorLayoutModel &layout)
{
    shell.Register("Editor.Viewport.SetSingleView", [&layout]() { layout.SetViewportCount(1); });
    shell.Register("Editor.Viewport.SetDoubleView", [&layout]() { layout.SetViewportCount(2); });
    shell.Register("Editor.Viewport.SetTripleView", [&layout]() { layout.SetViewportCount(3); });
    shell.Register("Editor.Viewport.SetQuadView",   [&layout]() { layout.SetViewportCount(4); });
    shell.Register("Editor.Viewport.ToggleTabVisibility", [&layout]() { layout.ToggleShowViewportDocktabs(); });
}

void InkBlueEditorApp::RegisterServicesShellCommands(EditorHost &host)
{
    host.RegisterShellCommandsForServices();
}
