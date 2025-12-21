//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

struct FViewportPanelContext
{
    const char* panelKey = nullptr;

    float width = 0.f;
    float height = 0.f;

    float rectMinX = 0.f;
    float rectMinY = 0.f;

    bool bFocused = false;
    bool bHovered = false;
    bool bOverViewport = false;

    // Mouse position in panel-local pixels
    float mouseX = 0.f;
    float mouseY = 0.f;
};
