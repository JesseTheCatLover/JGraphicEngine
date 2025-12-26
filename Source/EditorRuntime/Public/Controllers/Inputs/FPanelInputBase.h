//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

struct FPanelInputBase
{
    const char* panelKey = nullptr;

    float width = 0.f;
    float height = 0.f;

    bool bFocused = false;       // window focus (root)
    bool bHovered = false;       // window hovered
    bool bOverViewport = false;  // mouse over the Image (or Dummy)

    // Mouse position in panel-local pixels
    float mouseX = 0.f;
    float mouseY = 0.f;

    float rectMinX = 0.f;
    float rectMinY = 0.f;
};