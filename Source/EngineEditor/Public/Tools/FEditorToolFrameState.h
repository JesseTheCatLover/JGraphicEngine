//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include "Utilities/UDynamicID.h"

struct FCameraViewState
{
    float width = 0.f;
    float height = 0.f;
    float aspect = 16.f / 9.f;
    int viewIndex = 0;  // used to tag views
};

struct FCameraToolState
{
    // Which camera should respond to input this frame
    UDynamicID::IDType activeCameraId = UDynamicID::InvalidID;

    // Per camera: view properties (size / aspect / index)
    std::unordered_map<UDynamicID::IDType, FCameraViewState> viewstateMap;

    // Sample count per camera
    std::unordered_map<UDynamicID::IDType, int> cameraSampleMap;
};

struct FEditorToolFrameState
{
    FCameraToolState camera;
};
