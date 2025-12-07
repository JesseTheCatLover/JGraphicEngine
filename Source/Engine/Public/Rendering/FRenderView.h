//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "EViewType.h"
#include "ICameraViewSource.h"
#include "RHandles.h"
#include "Core/Math/FVector4.h"

class JScene;

struct FRenderView
{
    // Which scene to render (can be null for pure preview)
    JScene* scene = nullptr;

    // Camera for view/projection
    ICameraViewSource* camera = nullptr;

    // Type of this view
    EViewType viewType = EViewType::GameView;

    // Index of the view
    uint32_t viewIndex = 0;

    // Render target
    RFramebufferHandle targetFBO{};

    // Viewport rectangle (in target FBO space)
    int viewportX = 0;
    int viewportY = 0;
    int viewportW = 0;
    int viewportH = 0;

    // MSAA samples for this view's render
    int sampleCount = 1;   // 1 = no MSAA, 2/4/8/… = MSAA

    // Clear flags
    bool bClearColor = true;
    bool bClearDepth = true;
    FVector4 clearColorValue = {0.f, 0.f, 0.f, 1.f};

    uint32_t renderMask = 0xFFFFFFFFu;

    // Post-processing profile
    bool bEnablePostProcess = true;
    uint32_t postProfileId = 0; // 0 = default
};
