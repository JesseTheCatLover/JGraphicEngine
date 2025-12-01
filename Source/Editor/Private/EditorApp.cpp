#include "EditorApp.h"

#include <Core/CoreMinimal.h>
#include <Rendering/IPlatformSurface.h>
#include <GLFW/glfw3.h>
#include <EditorContext.h>
#include <ImGuiLayer.h>
#include <iostream>
#include <UI/Panels/SceneHierarchyPanel.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "Core/EditorCore.h"
#include "Layout/DockSpace.h"
#include "UI/IEditorPanels.h"
#include "UI/Panels/SceneViewportPanel.h"

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
    // Engine has already rendered the 3D scene by now.
    // Here we build the full editor UI.

    ImGuiIO& io = ImGui::GetIO();
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // ---- 1. Main menu bar (File/Edit/View/Window/Help) ----
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            // TODO: New Scene, Open, Save, etc.
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (m_Core)
            {
                bool canUndo = m_Core->CanUndo();
                bool canRedo = m_Core->CanRedo();

                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
                    m_Core->Undo();
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo))
                    m_Core->Redo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            // View toggles: panels, layout presets, etc.
            // We'll wire panel visibility here later.
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            // Window management / layout reset
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            // About, Docs, etc.
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
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##Toolbar", nullptr, toolbarFlags);

    if (ImGui::Button("Load"))  { /* */ }
    ImGui::SameLine();
    if (ImGui::Button("Save"))  { /* */ }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Transform tools (Select / Move / Rotate / Scale)
    static int gCurrentTool = 0;
    const char* toolNames[] = { "Select", "Move", "Rotate", "Scale" };
    for (int i = 0; i < 4; ++i)
    {
        if (i > 0) ImGui::SameLine();
        bool selected = (gCurrentTool == i);
        if (ImGui::Selectable(toolNames[i], selected, 0, ImVec2(0, 0)))
        {
            gCurrentTool = i;
            // TODO: tell EditorCore about current gizmo tool
        }
    }

    // Right side: toggles for panels (e.g. Asset Browser)
    ImGui::SameLine(ImGui::GetWindowWidth() - 150.0f);
    ImGui::TextUnformatted("Panels:");

    ImGui::SameLine();
    if (ImGui::Button("Assets"))
    {
        // toggle asset browser panel visibility
        //m_ShowAssetBrowser = !m_ShowAssetBrowser;
    }

    // You can add icons instead of text later (FontAwesome / custom icon font).
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
    if (m_Context)
    {
        for (auto& panel : m_Panels)
        {
            if (!panel) continue;

            // // Example: only draw asset browser if flag is on
            // if (panel->GetName() == std::string("Asset Browser") && !m_ShowAssetBrowser)
            //     continue;

            panel->Draw(*m_Context, *m_Core);
        }
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
    m_Core.reset();
    m_Context.reset();

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

    m_Context = MakeUnique<EditorContext>();

    // EngineEditor (safe bridge API)
    m_EngineEditor = TUniquePtr<EngineEditor>(new EngineEditor());

    // Create EditorCore to drive context & commands
    m_Core = MakeUnique<EditorCore>(*m_Context, *m_EngineEditor);

    // Register panels
    m_Panels.emplace_back(MakeUnique<SceneHierarchyPanel>());
    m_Panels.emplace_back(MakeUnique<SceneViewportPanel>());

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
    if (m_Core)
        m_Core->Update(deltaTime);
}
