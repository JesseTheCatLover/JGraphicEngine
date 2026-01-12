//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorApp.h"

#include <Core/CoreMinimal.h>
#include <Rendering/IPlatformSurface.h>
#include <GLFW/glfw3.h>
#include <ImGuiLayer.h>
#include <iostream>
#include <UI/Panels/SceneHierarchyPanel.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "Core/EditorHost.h"
#include "Layout/DockSpace.h"
#include "UI/IEditorPanels.h"
#include "UI/Panels/ViewportPanel.h"

EditorApp::EditorApp()
{
}

EditorApp::~EditorApp() = default;

void EditorApp::BeginFrame()
{
    m_ImGuiLayer->BeginFrame();
}

void EditorApp::RenderPanels()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // ---- 1. Main menu bar (File/Edit/View/Window/Help) ----
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {

            }
            if (ImGui::MenuItem("Save As", "Ctrl+Alt+s"))
            {

            }
            if (ImGui::MenuItem("Save All", "Ctrl+Shift+s"))
            {

            }
            ImGui::Separator();
            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
            {

            }
            if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
            {

            }
            ImGui::Separator();
            if (ImGui::MenuItem("New Project"))
            {

            }
            if (ImGui::MenuItem("Open Project"))
            {

            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (m_EditorHost)
            {
                // bool canUndo = m_EditorHost->CanUndo();
                // bool canRedo = m_EditorHost->CanRedo();

                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, true))
                {

                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, true))
                {

                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            // View toggles: panels, layout presets, etc.
            // We'll wire panel visibility here later.
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Viewport"))
        {
            if (ImGui::BeginMenu("Multi-View Modes", "Ctrl+M+V"))
            {
                static int sViewportCount = 1;
                if (ImGui::MenuItem("Single View", "Ctrl+V+1"))
                {
                    for (int i = 1; i < sViewportCount; i++)
                    {
                        m_Panels.pop_back();
                    }
                    sViewportCount = 1;
                }
                if (ImGui::MenuItem("Double View", "Ctrl+V+2"))
                {
                    for (int i = 1; i < sViewportCount; i++)
                    {
                        m_Panels.pop_back();
                    }

                    m_Panels.emplace_back(MakeUnique<ViewportPanel>(1))->OnCreate(*m_EditorHost);
                    sViewportCount = 2;
                }
                if (ImGui::MenuItem("Triple View", "Ctrl+V+3"))
                {
                    for (int i = 1; i < sViewportCount; i++)
                    {
                        m_Panels.pop_back();
                    }

                    m_Panels.emplace_back(MakeUnique<ViewportPanel>(1))->OnCreate(*m_EditorHost);
                    m_Panels.emplace_back(MakeUnique<ViewportPanel>(2))->OnCreate(*m_EditorHost);
                    sViewportCount = 3;
                }
                if (ImGui::MenuItem("Quad View", "Ctrl+V+4"))
                {
                    for (int i = 1; i < sViewportCount; i++)
                    {
                        m_Panels.pop_back();
                    }

                    m_Panels.emplace_back(MakeUnique<ViewportPanel>(1))->OnCreate(*m_EditorHost);
                    m_Panels.emplace_back(MakeUnique<ViewportPanel>(2))->OnCreate(*m_EditorHost);
                    m_Panels.emplace_back(MakeUnique<ViewportPanel>(3))->OnCreate(*m_EditorHost);
                    sViewportCount = 4;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::BeginMenu("Docking Layout"))
            {
                if (ImGui::MenuItem("Apply User Defaults"))
                {

                }

                if (ImGui::MenuItem("Load Layout File"))
                {

                }
                if (ImGui::BeginMenu("Recent Layouts"))
                {
                    ImGui::EndMenu();
                }

                ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

                if (ImGui::MenuItem("Reset To Editor Defaults"))
                {

                }

                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {

            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // ---- 2. Top toolbar window (below main menu bar) ----

    // Position toolbar at the top, just below main menu, full width
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 32.0f)); // 32 px tall tool strip
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGuiWindowFlags toolbarFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoDocking;

    ImGui::Begin("##Toolbar", nullptr, toolbarFlags);


    ImGui::End();
    ImGui::PopStyleVar(3);

    // ---- 3. Dockspace filling the rest ----

    // Reserve the top space: menu bar height + toolbar height
    float topOffset = ImGui::GetFrameHeight() + 32.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + topOffset));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - topOffset));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags dockspaceFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpaceRoot", nullptr, dockspaceFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceID = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    // ---- 4. Draw panels inside dockspace ----

    for (auto& panel : m_Panels)
    {
        if (!panel) continue;

        panel->Draw( *m_EditorHost);
    }

    ImGui::End(); // DockSpaceRoot
}


void EditorApp::EndFrame()
{
    if (m_ImGuiLayer)
        m_ImGuiLayer->EndFrame();
}

void EditorApp::Shutdown()
{
    // Destroy panels first
    m_Panels.clear();

    // Destroy core/context
    m_EditorHost.reset();

    if (m_ImGuiLayer)
    {
        m_ImGuiLayer->Shutdown();
        m_ImGuiLayer.reset();
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
    m_ImGuiLayer = MakeUnique<ImGuiLayer>(window);


    // EditorRuntime (safe bridge API to Engine)
    m_EditorRuntime = TUniquePtr<EditorRuntime>(new EditorRuntime());

    // Create EditorCore to drive context & commands
    m_EditorHost = TUniquePtr<EditorHost>(new EditorHost(*m_EditorRuntime));

    // Register panels
    m_Panels.emplace_back(MakeUnique<SceneHierarchyPanel>());
    m_Panels.emplace_back(MakeUnique<ViewportPanel>(0));

    // Call OnCreate for all panels now that Context/Core exist
    for (auto& panel : m_Panels)
    {
        if (panel)
            panel->OnCreate(*m_EditorHost);
    }

    m_DockSpace = MakeUnique<DockSpace>();
}

void EditorApp::OnSceneLoaded(const std::string &sceneName)
{
}

void EditorApp::OnRenderOverlay()
{
    BeginFrame();
    RenderPanels();
    EndFrame();
}

void EditorApp::OnTick(float deltaTime)
{
    if (m_EditorHost)
        m_EditorHost->Tick(deltaTime);
}
