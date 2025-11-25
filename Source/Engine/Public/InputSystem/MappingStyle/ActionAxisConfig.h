//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "InputSystem/FInputDeviceState.h"
#include "InputSystem/InputChannels.h"

struct FInputBinding
{
    EInputDeviceType deviceType; // Keyboard / Mouse / Gamepad
    int deviceIndex; // usually 0
    int code;        // keycode / button index / axis index

    float  scale    = 1.0f;
    float  deadZone = 0.0f;
    bool   invert   = false;
};

// One logical action/axis exposed to gameplay
struct FActionAxisSlot
{
    std::string name;   // "Jump", "Move", "Look"
    EInputChannelType type;   // Bool / Axis1D / Axis2D
    std::vector<FInputBinding> bindings;
};

// A map = list of actions (what will be serialize as a .jasset)
struct FActionAxisMap
{
    std::vector<FActionAxisSlot> actions;
};