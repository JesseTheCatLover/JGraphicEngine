//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ViewportPanel.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "Controllers/Outputs/FViewportOutput.h"
#include "Core/EditorHost.h"
#include "Core/EngineGlobals.h"
#include "Subsystems/ViewportSubsystem.h"

void ViewportPanel::OnCreate(EditorHost&) {}

void ViewportPanel::OnDestroy(EditorHost& host)
{
    host.GetSubsystem<ViewportSubsystem>().Destroy(GetPanelKey()); // TODO: Should be handled by a lower level pipeline
}

const char* ViewportPanel::GetPanelKey() const
{
    return m_PanelKey.c_str();
}
void ViewportPanel::Draw(EditorHost& host)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    const bool bVisible = ImGui::Begin(GetName()); // returns false for collapsed/hidden/inactive dock tab
    if (!bVisible)
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }


    // Size of viewport
    ImVec2 size = ImGui::GetContentRegionAvail();
    float w = (size.x > 0.f) ? size.x : 0.f;
    float h = (size.y > 0.f) ? size.y : 0.f;


    // Ask for current texture (might be null on first frame)
    const FViewportOutput* out = host.GetSubsystem<ViewportSubsystem>().GetOutput(GetPanelKey());

    if(out && out->bHasTexture)
    {
        ImVec2 uv0(0.0f, 1.0f);
        ImVec2 uv1(1.0f, 0.0f);
        ImGui::Image((ImTextureID)out->nativeTexture, ImVec2(w, h), uv0, uv1);
    }
    else
    {
        ImGui::Dummy(ImVec2(w, h));
        ImVec2 cursor = ImGui::GetCursorScreenPos(); // We want stable rect+mouse even if texture isn't ready
        ImGui::SetCursorScreenPos(cursor);
        ImGui::TextUnformatted("No valid scene texture available.");
    }

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

    // Submit panel input
    FViewportPanelInput input{};
    input.panelKey = GetPanelKey();
    input.width = w;
    input.height = h;
    input.bFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    input.bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    input.bLeftClicked    = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    input.bLeftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    input.bRightClicked   = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    input.bRightReleased  = ImGui::IsMouseReleased(ImGuiMouseButton_Right);

    ImGuiIO& io = ImGui::GetIO();
    input.bCtrl  = io.KeyCtrl || io.KeySuper;
    input.bShift = io.KeyShift;
    input.bAlt   = io.KeyAlt;
    input.bSuper = io.KeySuper;

    input.bAppFocused = !io.AppFocusLost;

    const bool windowCollapsed = ImGui::IsWindowCollapsed();
    const bool windowHiddenByClipping = ImGui::GetCurrentWindow()->Hidden;
    const bool tooSmall = (w <= 1.0f || h <= 1.0f);

    input.bHidden = windowCollapsed || windowHiddenByClipping || tooSmall;

    // Is mouse over the viewport image area?
    const bool bOverViewport = ImGui::IsItemHovered(ImGuiHoveredFlags_None);
    input.bOverViewport = bOverViewport;
    input.mouseX = mouseX;
    input.mouseY = mouseY;

    input.rectMinX = rectMin.x;
    input.rectMinY = rectMin.y;

    host.GetSubsystem<ViewportSubsystem>().SubmitInput(input);

    ImGui::End();
    ImGui::PopStyleVar();
}