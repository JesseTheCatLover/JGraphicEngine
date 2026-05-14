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

struct FWindowDesc // TODO: Implement bResizable
{
    int width = 1280;
    int height = 720;
    bool bvSync = true;
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