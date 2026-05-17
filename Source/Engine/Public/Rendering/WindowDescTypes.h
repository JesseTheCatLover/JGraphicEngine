//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <functional>
#include <vector>

enum class EWindowState
{
    Normal,
    Minimized,
    Maximized,
    Fullscreen,
    Hidden,
    Lost
};

struct FWindowDesc
{
    int width = 1280;
    int height = 720;

    bool bvSync = true;

    bool bResizable = true; ///< Can the window be resized by the user?

    int minWidth = 640; ///< Minimum width clamp (ignored if <= 0)
    int minHeight = 480; ///< Minimum height clamp (ignored if <= 0)

    int maxWidth = 0; ///< Maximum width clamp (ignored if <= 0)
    int maxHeight = 0; ///< Maximum height clamp (ignored if <= 0)

    EWindowState windowState = EWindowState::Maximized;
    std::string title = "JGraphXEngine";
    void* nativeHandle = nullptr;   // backend-specific
    void* monitorHandle = nullptr;  // display/output, backend-specific
};

enum class ECursorMode
{
    Visible,
    Hidden,
    Disabled
};