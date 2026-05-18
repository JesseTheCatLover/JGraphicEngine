//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ImGuiRenderer.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "Core/EditorHost.h"
#include "EditorRuntime.h"
#include "Layout/EditorLayoutModel.h"
#include "../Core/EditorAssetCache.h"
#include "Core/Services/EditTimelineService.h"
#include "Core/Services/HotkeyService.h"
#include "Core/Services/ShellCommandService.h"
#include "UI/IEditorPanel.h"

static bool IsViewportKey(const char* key)
{
    return (std::strncmp(key, "Viewport", 8) == 0);
}

void ImGuiRenderer::Initialize(EditorHost& host, EditorRuntime& runtime, EditorLayoutModel& layout, EditorAssetCache& cache)
{
    m_Host = &host;
    m_Runtime = &runtime;
    m_Layout = &layout;
    m_Cache = &cache;

    // Tools dockspace: accept tool windows
    m_ToolsDockClass = {};
    m_ToolsDockClass.ClassId = ImHashStr("DockClass_Tools");
    m_ToolsDockClass.DockingAllowUnclassed = false;

    // Viewport dockspace: only viewports may dock here
    m_ViewportDockClass = {};
    m_ViewportDockClass.ClassId = ImHashStr("DockClass_Viewports");
    m_ViewportDockClass.DockingAllowUnclassed = false;

    // ViewportDockHost: MUST use same ClassId as Tools so it can dock with tool panels.
    m_ViewportHostDockClass = m_ToolsDockClass; // copies ClassId + settings


    // Hide the dock tab bar for the node containing this window
    m_ViewportHostDockClass.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoTabBar;


    // Optional: prevent other windows docking "over" it
    m_ViewportHostDockClass.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverMe;
}

void ImGuiRenderer::RenderChrome(float deltaTime)
{
    DrawMainMenuBar();
    DrawToolbar();
    DrawDockspaceAndPanels(deltaTime);
}

void ImGuiRenderer::RenderPanels(std::span<IEditorPanel * const> panels)
{
    for (IEditorPanel* p : panels)
    {
        if (!p) continue;

        if (const ImGuiWindowClass* wc = GetDockClassForPanel(*p))
            ImGui::SetNextWindowClass(wc);

        p->Draw(*m_Host);
    }
}

void ImGuiRenderer::DrawMainMenuBar()
{
    if (!ImGui::BeginMainMenuBar() || !m_Host)
        return;

    auto& shell = m_Host->GetService<ShellCommandService>();
    auto& hotkeys = m_Host->GetService<HotkeyService>();

    if (ImGui::BeginMenu("File"))
    {
        ImGui::MenuItem("Save", "Ctrl+S");
        ImGui::MenuItem("Save As", "Ctrl+Alt+S");
        ImGui::MenuItem("Save All", "Ctrl+Shift+S");
        ImGui::Separator();
        ImGui::MenuItem("New Scene", "Ctrl+N");
        ImGui::MenuItem("Open Scene", "Ctrl+O");
        ImGui::Separator();
        ImGui::MenuItem("New Project");
        ImGui::MenuItem("Open Project");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        const bool canUndo = m_Host->GetService<EditTimelineService>().CanUndo();
        const bool canRedo = m_Host->GetService<EditTimelineService>().CanRedo();

        const std::string hkUndo = hotkeys.GetShortcutText("Editor.History.Undo");
        if (ImGui::MenuItem("Undo", hkUndo.empty() ? nullptr : hkUndo.c_str(), false, canUndo))
            shell.Execute("Editor.History.Undo");

        const std::string hkRedo = hotkeys.GetShortcutText("Editor.History.Redo");
        if (ImGui::MenuItem("Redo", hkRedo.empty() ? nullptr : hkRedo.c_str(), false, canRedo))
            shell.Execute("Editor.History.Redo");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        bool bHierarchy = m_Layout->IsPanelVisible(EEditorPanelType::SceneHierarchy);
        if (ImGui::MenuItem("Scene Hierarchy",
            hotkeys.GetShortcutText("Editor.View.ToggleSceneHierarchy").c_str(), bHierarchy))
            shell.Execute("Editor.View.ToggleSceneHierarchy");

        bool bConsole = m_Layout->IsPanelVisible(EEditorPanelType::Console);
        if (ImGui::MenuItem("Console",
            hotkeys.GetShortcutText("Editor.View.ToggleConsole").c_str(), bConsole))
            shell.Execute("Editor.View.ToggleConsole");

        bool bAssetBrowser = m_Layout->IsPanelVisible(EEditorPanelType::AssetBrowser);
        if (ImGui::MenuItem("Asset Browser",
            hotkeys.GetShortcutText("Editor.View.ToggleAssetBrowser").c_str(), bAssetBrowser))
            shell.Execute("Editor.View.ToggleAssetBrowser");

        bool bInspector = m_Layout->IsPanelVisible(EEditorPanelType::Inspector);
        if (ImGui::MenuItem("Inspector",
            hotkeys.GetShortcutText("Editor.View.ToggleInspector").c_str(), bInspector))
            shell.Execute("Editor.View.ToggleInspector");

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Viewport"))
    {
        if (ImGui::BeginMenu("Multi-View Modes", "Ctrl+M+V"))
        {
            if (ImGui::MenuItem("Single View",
                hotkeys.GetShortcutText("Editor.Viewport.SetSingleView").c_str()))
                shell.Execute("Editor.Viewport.SetSingleView");

            if (ImGui::MenuItem("Double View",
                hotkeys.GetShortcutText("Editor.Viewport.SetDoubleView").c_str()))
                shell.Execute("Editor.Viewport.SetDoubleView");

            if (ImGui::MenuItem("Triple View",
                hotkeys.GetShortcutText("Editor.Viewport.SetTripleView").c_str()))
                shell.Execute("Editor.Viewport.SetTripleView");

            if (ImGui::MenuItem("Quad View",
                hotkeys.GetShortcutText("Editor.Viewport.SetQuadView").c_str()))
                shell.Execute("Editor.Viewport.SetQuadView");

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Toggle Tab Visibility",
                hotkeys.GetShortcutText("Editor.Viewport.ToggleTabVisibility").c_str()))
        {
            shell.Execute("Editor.Viewport.ToggleTabVisibility");
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window"))
    {
        if (ImGui::BeginMenu("Docking Layout"))
        {
            ImGui::MenuItem("Apply User Defaults");
            ImGui::MenuItem("Load Layout File");
            if (ImGui::BeginMenu("Recent Layouts"))
            {
                ImGui::MenuItem("No recent file available", 0, false, false);
                ImGui::EndMenu();
            }
            ImGui::Separator();
            ImGui::MenuItem("Reset To Editor Defaults");
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        ImGui::MenuItem("About");
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void ImGuiRenderer::DrawToolbar()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 32.0f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));
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

    // Example: draw a few cached icons if they exist
    // Keys correspond to files under Assets/Editor/Textures** (without extension)
    // e.g. Assets/Editor/Textures/Toolbar/Save.png -> "Toolbar/Save"
    const char* keys[] = { "Toolbar/Save", "Toolbar/Run", "Toolbar/Translate", "Toolbar/Rotate", "Toolbar/Scale" };

    ImVec2 uv0(0.0f, 1.0f);
    ImVec2 uv1(1.0f, 0.0f);

    for (const char* k : keys)
    {
        RTextureHandle h = m_Cache->GetTexture(k);
        if (!h.IsValid())
            continue;

        ImGui::SameLine();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float hh = (avail.y > 0.f) ? avail.y : 0.f;

        auto texId = (ImTextureID)m_Runtime->GetViewport().GetNativeTexture(h);
        ImGui::Image(texId, ImVec2(hh, hh), uv0, uv1);
    }

    std::string gizmoMode = "GizmoMode: ";
    switch (m_Host->GetSubsystem<ViewportSubsystem>().GetGizmoMode())
    {
        case GizmoEditorTool::EMode::Translate: gizmoMode += "Translate"; break;
        case GizmoEditorTool::EMode::Scale:     gizmoMode += "Scale"; break;
        case GizmoEditorTool::EMode::Rotate:    gizmoMode += "Rotate"; break;
        default: break;
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(gizmoMode.c_str());

    std::string gizmoSpace = "GizmoSpace: ";
    switch (m_Host->GetSubsystem<ViewportSubsystem>().GetGizmoSpace())
    {
        case GizmoEditorTool::ESpace::World: gizmoSpace += "World"; break;
        case GizmoEditorTool::ESpace::Local: gizmoSpace += "Local"; break;
        default: break;
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::TextUnformatted(gizmoSpace.c_str());

    ImGui::End();
    ImGui::PopStyleVar(4);
}

void ImGuiRenderer::DrawDockspaceAndPanels(float /*deltaTime*/)
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    float topOffset = ImGui::GetFrameHeight() + 32.0f;


    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + topOffset));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - topOffset));
    ImGui::SetNextWindowViewport(viewport->ID);


    ImGuiWindowFlags rootFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;


    ImGuiDockNodeFlags toolsDockFlags = ImGuiDockNodeFlags_PassthruCentralNode;


    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));


    ImGui::Begin("DockSpaceRoot", nullptr, rootFlags);
    ImGui::PopStyleVar(3);


    // 1) ROOT / TOOLS DOCKSPACE
    ImGuiID toolsDockspaceID = ImGui::GetID("ToolsDockSpace");
    ImGui::DockSpace(toolsDockspaceID, ImVec2(0, 0), toolsDockFlags, &m_ToolsDockClass);


    // 2) VIEWPORT DOCK HOST WINDOW (this window docks into ToolsDockSpace)
    // Make this window a "tool-class" participant so it can live in the main layout.
    ImGui::SetNextWindowClass(&m_ViewportHostDockClass);


    // Optional: ensure first time it goes to the central node of tools dockspace.
    // (Works nicely with DockBuilder too.)
    ImGui::SetNextWindowDockID(toolsDockspaceID, ImGuiCond_FirstUseEver);


    ImGuiWindowFlags vpHostFlags =
            ImGuiWindowFlags_NoTitleBar | // no header/title
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;

    // IMPORTANT: remove inner padding so the dockspace fills it perfectly
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    // A normal window that contains a nested dockspace:
    if (ImGui::Begin("ViewportDockHost", nullptr, vpHostFlags))
    {
        ImGuiDockNodeFlags vpDockFlags = ImGuiDockNodeFlags_None;
        if (!m_Layout->GetShowViewportDocktabs())
            vpDockFlags |= ImGuiDockNodeFlags_NoTabBar;


        ImGuiID viewportDockspaceID = ImGui::GetID("ViewportDockSpace");
        ImGui::DockSpace(viewportDockspaceID, ImVec2(0, 0), vpDockFlags, &m_ViewportDockClass);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);

    ImGui::End();
}

const ImGuiWindowClass* ImGuiRenderer::GetDockClassForPanel(const IEditorPanel& panel)
{
    if (panel.GetDockGroup() == EPanelDockGroup::Viewport)
        return &m_ViewportDockClass;


    // Everything else (hierarchy/inspector/console/etc.)
    return &m_ToolsDockClass;
}