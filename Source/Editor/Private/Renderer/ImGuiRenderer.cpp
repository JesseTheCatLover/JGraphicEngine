//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ImGuiRenderer.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "Core/EditorHost.h"
#include "EditorRuntime.h"
#include "Layout/EditorLayoutModel.h"
#include "../Core/EditorAssetCache.h"
#include "UI/IEditorPanels.h"

void ImGuiRenderer::Initialize(EditorHost& host, EditorRuntime& runtime, EditorLayoutModel& layout, EditorAssetCache& cache)
{
    m_Host = &host;
    m_Runtime = &runtime;
    m_Layout = &layout;
    m_Cache = &cache;
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
        if (p) p->Draw(*m_Host);
}

void ImGuiRenderer::DrawMainMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

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
        if (m_Host)
        {
            ImGui::MenuItem("Undo", "Ctrl+Z", false, true);
            ImGui::MenuItem("Redo", "Ctrl+Y", false, true);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        // Uses LayoutModel (single panels)
        bool hier = m_Layout->IsPanelVisible(EEditorPanelType::SceneHierarchy);
        if (ImGui::MenuItem("Scene Hierarchy", "X", hier))
            m_Layout->TogglePanelVisibility(EEditorPanelType::SceneHierarchy);

        bool insp = m_Layout->IsPanelVisible(EEditorPanelType::Inspector);
        if (ImGui::MenuItem("Inspector", "V", insp))
            m_Layout->TogglePanelVisibility(EEditorPanelType::Inspector);

        bool ab = m_Layout->IsPanelVisible(EEditorPanelType::AssetBrowser);
        if (ImGui::MenuItem("Asset Browser", "C", ab))
            m_Layout->TogglePanelVisibility(EEditorPanelType::AssetBrowser);

        bool con = m_Layout->IsPanelVisible(EEditorPanelType::Console);
        if (ImGui::MenuItem("Console", "Z", con))
            m_Layout->TogglePanelVisibility(EEditorPanelType::Console);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Viewport"))
    {
        if (ImGui::BeginMenu("Multi-View Modes", "Ctrl+M+V"))
        {
            if (ImGui::MenuItem("Single View", "Ctrl+V+1")) m_Layout->SetViewportCount(1);
            if (ImGui::MenuItem("Double View", "Ctrl+V+2")) m_Layout->SetViewportCount(2);
            if (ImGui::MenuItem("Triple View", "Ctrl+V+3")) m_Layout->SetViewportCount(3);
            if (ImGui::MenuItem("Quad View",   "Ctrl+V+4")) m_Layout->SetViewportCount(4);

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Toggle Tab Visibility", "Ctrl+V+H"))
            m_ShowViewportDockTabs = !m_ShowViewportDockTabs;

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

void ImGuiRenderer::DrawDockspaceAndPanels(float dt)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

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

    // Optionally hide tabs
    ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    if (!m_ShowViewportDockTabs)
        dockFlags |= ImGuiDockNodeFlags_NoTabBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpaceRoot", nullptr, dockspaceFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceID = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockFlags);
    ImGui::End();
}
