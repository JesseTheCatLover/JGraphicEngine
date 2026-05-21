//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "UI/Themes/ImGuiTheme.h"

namespace ImGuiTheme
{
    void ApplyEditorTheme(const FThemeOptions& opt)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();

        if (opt.enableDocking)
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        else
            io.ConfigFlags &= ~ImGuiConfigFlags_DockingEnable;

        if (opt.noWindowMenuButton)
            style.WindowMenuButtonPosition = ImGuiDir_None;

        style.WindowRounding    = opt.windowRounding;
        style.FrameRounding     = opt.frameRounding;
        style.ScrollbarRounding = opt.scrollbarRounding;
        style.FramePadding      = opt.framePadding;
        style.ItemSpacing       = opt.itemSpacing;

        ImVec4* colors = style.Colors;

        ImVec4 bg_dark   = ImVec4(0.17f, 0.17f, 0.17f, 1.0f);
        ImVec4 bg_mid    = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        ImVec4 bg_light  = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
        ImVec4 text_col  = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);

        ImVec4 accent_active = ImVec4(0.34f, 0.42f, 0.50f, 1.0f);

        colors[ImGuiCol_Text]         = text_col;
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);

        colors[ImGuiCol_WindowBg]     = bg_dark;
        colors[ImGuiCol_ChildBg]      = bg_dark;
        colors[ImGuiCol_PopupBg]      = bg_mid;

        colors[ImGuiCol_Border]       = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        colors[ImGuiCol_TitleBg]          = bg_dark;
        colors[ImGuiCol_TitleBgActive]    = bg_mid;
        colors[ImGuiCol_TitleBgCollapsed] = bg_mid;

        colors[ImGuiCol_Button]        = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
        colors[ImGuiCol_ButtonActive]  = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);

        colors[ImGuiCol_HeaderHovered] = ImVec4(0.37f, 0.48f, 0.58f, 1.0f);
        colors[ImGuiCol_HeaderActive]  = accent_active;

        colors[ImGuiCol_Tab]                = bg_mid;
        colors[ImGuiCol_TabHovered]         = bg_light;
        colors[ImGuiCol_TabActive]          = bg_light;
        colors[ImGuiCol_TabUnfocused]       = bg_mid;
        colors[ImGuiCol_TabUnfocusedActive] = bg_light;

        style.TabRounding = 3.0f;

        // Remove the grip arrow
        ImVec4 oldGrip     = style.Colors[ImGuiCol_ResizeGrip];
        ImVec4 oldGripAct  = style.Colors[ImGuiCol_ResizeGripActive];
        ImVec4 oldGripHov  = style.Colors[ImGuiCol_ResizeGripHovered];

        style.Colors[ImGuiCol_ResizeGrip]        = ImVec4(0, 0, 0, 0);
        style.Colors[ImGuiCol_ResizeGripActive]  = ImVec4(0, 0, 0, 0);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0, 0, 0, 0);
    }

    bool TryLoadDefaultFontFromFile(const std::string& ttfPath, float sizePx)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImFont* f = io.Fonts->AddFontFromFileTTF(ttfPath.c_str(), sizePx);
        if (!f) return false;
        io.FontDefault = f;
        return true;
    }
}
