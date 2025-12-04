//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SceneViewportPanel.h"

#include "imgui.h"
#include "Core/EditorCore.h"

void SceneViewportPanel::OnCreate(EditorContext &context, EditorCore &core)
{
    core.CreateCameraForPanel(this);
}

void SceneViewportPanel::OnDestroy(EditorContext &context, EditorCore &core)
{
    core.DestroyCameraForPanel(this);
}

void SceneViewportPanel::Draw(EditorContext& context, EditorCore& core)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (!ImGui::Begin(GetName()))
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    m_IsFocused = ImGui::IsWindowFocused();
    m_IsHovered = ImGui::IsWindowHovered();

    // Get ratio
    ImVec2 size = ImGui::GetContentRegionAvail();
    if (m_Width != size.x || m_Height != size.y)
    {
        m_Width = size.x;
        m_Height = size.y;
        float aspect = (m_Height > 0) ? m_Width / m_Height : 16.0f / 9.0f;

        // Notify core so it can update the camera aspect
        core.OnViewportResized(this, aspect);
        core.SetSizeTemp(m_Width, m_Height);
    }

    void* native = core.GetViewportTextureHandle();
    if (!native)
    {
        ImGui::TextUnformatted("No valid scene texture available.");
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }
    ImTextureID texID = (ImTextureID)native;

    // 3) Draw it. Flip Y if needed (OpenGL’s origin is bottom-left)
    ImVec2 uv0(0.0f, 1.0f);
    ImVec2 uv1(1.0f, 0.0f);

    ImGui::Image(texID, size, uv0, uv1);

     // The last item (the Image) is the viewport area.
    bool overViewport = ImGui::IsItemHovered(ImGuiHoveredFlags_None);
    bool leftDown     = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool leftClicked  = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool leftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

    // Activate when you click and hold inside the viewport
    if (overViewport && leftClicked && m_IsFocused)
    {
        core.SetViewportFocused(this, true);
    }

    // Deactivate when:
    //  - Left mouse is released, OR
    //  - Window lost focus
    if (leftReleased || !m_IsFocused)
    {
        core.SetViewportFocused(this, false);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}