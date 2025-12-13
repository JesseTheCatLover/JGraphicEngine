//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SceneViewportPanel.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "Core/EditorCore.h"
#include "Core/FSelectionModifiers.h"

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

    // Make this window's dock node hide its tab bar (headless viewport)
    // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
    // {
    //     ImGuiID dockId = ImGui::GetWindowDockID();
    //     if (dockId != 0)
    //     {
    //         if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockId))
    //         {
    //             // Hide tab bar when this node has a single window.
    //             node->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar;
    //
    //             // "never show tabs" even if multiple windows get docked here:
    //             // node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
    //         }
    //     }
    // }

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
    bool leftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool rightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    bool rightReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right);

    // Actor picking: single-click selects actor under mouse
    if (overViewport && leftClicked)
    {
        // Mouse in absolute screen coords
        ImVec2 mousePos = ImGui::GetMousePos();

        // Rect of the Image in screen coords
        ImVec2 rectMin = ImGui::GetItemRectMin();
        ImVec2 rectMax = ImGui::GetItemRectMax();

        // Mouse position relative to viewport image
        ImVec2 local;
        local.x = mousePos.x - rectMin.x;
        local.y = mousePos.y - rectMin.y;

        // Clamp defensively into [0, width/height]
        local.x = ImClamp(local.x, 0.0f, rectMax.x - rectMin.x);
        local.y = ImClamp(local.y, 0.0f, rectMax.y - rectMin.y);

        ImGuiIO& io = ImGui::GetIO();
        FSelectionModifiers mods;
        mods.bRange = false;
        mods.bToggle = io.KeyCtrl || io.KeySuper || io.KeyShift;

        // Tell editor core to perform picking in this viewport
        core.PickActorAtViewportPos(this, local.x, local.y, mods);
    }


    // Start capture: click inside the image
    if (overViewport && rightClicked)
    {
        ImGui::SetWindowFocus(); // TODO: Make the windows lockable to gain focus for future. (Optional setting)
        m_HasMouseCapture = true;
        core.SetViewportFocused(this, true);
    }

    // End capture when left is released
    if (m_HasMouseCapture && rightReleased)
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