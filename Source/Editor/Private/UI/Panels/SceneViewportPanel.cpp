//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SceneViewportPanel.h"

#include "imgui.h"
#include "Core/EditorCore.h"

void SceneViewportPanel::OnCreate(EditorContext &context, EditorCore &core)
{
    core.CreateCameraForPanel(this);
    core.SetViewportMSAASamples(this, 4);
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

        // Notify core so it can update the camera aspect
        core.OnViewportResized(this, m_Width, m_Height);
    }

    void* native = core.GetViewportTextureHandle(this);
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
    bool leftClicked  = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool leftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

    // Start capture: click inside the image
    if (overViewport && leftClicked)
    {
        m_HasMouseCapture = true;
        core.SetViewportFocused(this, true);
    }

    // End capture when left is released
    if (m_HasMouseCapture && leftReleased)
    {
        m_HasMouseCapture = false;
        core.SetViewportFocused(this, false);
    }

    // Safety check: if the window is not visible anymore, drop capture
    if (!ImGui::IsWindowAppearing() && !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && m_HasMouseCapture)
    {
        m_HasMouseCapture = false;
        core.SetViewportFocused(this, false);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}