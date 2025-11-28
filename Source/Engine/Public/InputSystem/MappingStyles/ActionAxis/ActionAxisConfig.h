//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "InputSystem/EPhysicalInput.h"
#include "InputSystem/FInputDeviceState.h"
#include "InputSystem/InputChannels.h"

struct FInputBinding
{
    EInputDeviceType deviceType; // Keyboard / Mouse / Gamepad
    int deviceIndex = 0;

    EPhysicalInput input = EPhysicalInput::Unknown;

    float scale = 1.0f;
    float deadZone = 0.0f;
    bool invert = false;
};

// One logical action/axis exposed to gameplay
struct FActionAxisSlot
{
    std::string name;   // "Jump", "Move", "Look"
    EInputChannelType type;   // Bool / Axis1D / Axis2D
    std::vector<FInputBinding> bindings;
};

// A map = list of actions (this is what will be serialized as a .jasset)
struct FActionAxisMap
{
    std::vector<FActionAxisSlot> actions;
};