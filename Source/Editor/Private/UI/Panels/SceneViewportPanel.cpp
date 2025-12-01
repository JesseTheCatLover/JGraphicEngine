//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SceneViewportPanel.h"

#include "imgui.h"
#include "Core/EditorCore.h"

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

    ImVec2 size = ImGui::GetContentRegionAvail();
    m_Width  = size.x;
    m_Height = size.y;

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

    // later: camera input / picking using m_IsFocused/m_IsHovered

    ImGui::End();
    ImGui::PopStyleVar();
}