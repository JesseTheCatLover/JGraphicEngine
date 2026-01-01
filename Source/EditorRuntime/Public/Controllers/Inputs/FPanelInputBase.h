//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

struct FPanelInputBase
{
    const char* panelKey = nullptr;

    float width = 0.f;
    float height = 0.f;

    bool bAppFocused = true;     // OS/app focus (global)
    bool bFocused = false;       // window focus (root)
    bool bHovered = false;       // window hovered

    bool bHidden = false;        // tab/collapsed/culled/not visible this frame

    bool bLeftClicked = false;
    bool bLeftReleased = false;
    bool bRightClicked = false;
    bool bRightReleased = false;

    bool bCtrl = false;
    bool bShift = false;
    bool bAlt = false;
    bool bSuper = false;

    // Mouse position in panel-local pixels
    float mouseX = 0.f;
    float mouseY = 0.f;

    float rectMinX = 0.f;
    float rectMinY = 0.f;
};