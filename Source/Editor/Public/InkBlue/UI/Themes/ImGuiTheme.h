//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include "imgui.h"

namespace ImGuiTheme
{
    struct FThemeOptions
    {
        bool enableDocking = false;
        bool noWindowMenuButton = true;
        float windowRounding = 3.0f;
        float frameRounding  = 3.0f;
        float scrollbarRounding = 3.0f;
        ImVec2 framePadding = ImVec2(6, 4);
        ImVec2 itemSpacing  = ImVec2(10, 4);
    };

    void ApplyEditorTheme(const FThemeOptions& opt);

    bool TryLoadDefaultFontFromFile(const std::string& ttfPath, float sizePx = 16.0f);
}

