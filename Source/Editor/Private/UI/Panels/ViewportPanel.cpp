//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "ViewportPanel.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "Controllers/Outputs/FViewportOutput.h"
#include "Core/EditorHost.h"
#include "Core/EngineGlobals.h"
#include "Core/FSelectionModifiers.h"
#include "Subsystems/ViewportSubsystem.h"

void ViewportPanel::OnCreate(EditorHost&) {}

void ViewportPanel::OnDestroy(EditorHost& host)
{
}

const char* ViewportPanel::GetPanelKey() const
{
    return m_PanelKey.c_str();
}
void ViewportPanel::Draw(EditorHost& host)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (!ImGui::Begin(GetName()))
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    // Size of viewport image
    ImVec2 size = ImGui::GetContentRegionAvail();
    float w = (size.x > 0.f) ? size.x : 0.f;
    float h = (size.y > 0.f) ? size.y : 0.f;

    // Window focus/hover (input for viewport policy)
    const bool windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    const bool windowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    // We want stable rect+mouse even if texture isn't ready
    ImVec2 cursor = ImGui::GetCursorScreenPos();

    // Rect of the drawn item (Image or Dummy)
    ImVec2 rectMin = ImGui::GetItemRectMin();
    ImVec2 rectMax = ImGui::GetItemRectMax();

    // Mouse local position relative to viewport image
    ImVec2 mouse = ImGui::GetMousePos();
    float mouseX = mouse.x - rectMin.x;
    float mouseY = mouse.y - rectMin.y;

    // Clamp into viewport bounds
    mouseX = ImClamp(mouseX, 0.0f, rectMax.x - rectMin.x);
    mouseY = ImClamp(mouseY, 0.0f, rectMax.y - rectMin.y);

    // Is mouse over the viewport image area?
    const bool bOverViewport = ImGui::IsItemHovered(ImGuiHoveredFlags_None);

    // Submit panel input
    FViewportPanelInput input{};
    input.panelKey = GetPanelKey();
    input.width = w;
    input.height = h;
    input.bFocused = windowFocused;
    input.bHovered = windowHovered;
    input.bOverViewport = bOverViewport;
    input.mouseX = mouseX;
    input.mouseY = mouseY;

    input.rectMinX = rectMin.x;
    input.rectMinY = rectMin.y;

    host.GetViewportSubsystem().SubmitInput(input);


    // Draw (or dummy placeholder)
    ImVec2 uv0(0.0f, 1.0f);
    ImVec2 uv1(1.0f, 0.0f);

    // Ask for current texture (might be null on first frame)
    const FViewportOutput* out = host.GetViewportSubsystem().GetOutput(GetPanelKey());

    if(out && out->bHasTexture)
    {
        ImGui::Image((ImTextureID)out->nativeTexture, ImVec2(w, h), uv0, uv1);
    }
    else
    {
        ImGui::Dummy(ImVec2(w, h));
        ImGui::SetCursorScreenPos(cursor);
        ImGui::TextUnformatted("No valid scene texture available.");
    }


    ImGui::End();
    ImGui::PopStyleVar();
}