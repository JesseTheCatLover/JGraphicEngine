//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include "Utilities/UDynamicID.h"

struct FEditorToolFrameState
{
    // Camera domain

    UDynamicID::IDType activeCameraId = UDynamicID::InvalidID;
    std::unordered_map<UDynamicID::IDType, float> cameraAspectMap;
};
