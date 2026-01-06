//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include "Rendering/EViewType.h"
#include "Core/Math/FMatrix4.h"

class ICameraViewSource;

struct FViewParams
{
    EViewType viewType = EViewType::GameView;
    uint32_t viewIndex = 0; ///< index of the view
    ICameraViewSource* camera  = nullptr;

    FMatrix4 viewMatrix = FMatrix4::Identity();
    FMatrix4 projMatrix = FMatrix4::Identity();
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    uint32_t renderMask = 0xFFFFFFFFu; // used for filtering
};
