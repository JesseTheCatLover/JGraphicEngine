//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

enum class ERawInputType : uint8_t
{
    KeyDown, KeyUp,
    MouseMove,
    MouseButtonDown, MouseButtonUp,
    MouseWheel,
    GamepadButtonDown, GamepadButtonUp,
    GamepadAxis,
    TextInput,
};

struct FRawInputEvent
{
    ERawInputType type;

    int deviceID; ///< Which keyboard/mouse/gamepad
    uint32_t code; ///< underlying EPhysicalInput (casted)
    float value;  ///< axis value, wheel delta, etc.
    double timestamp; ///< in seconds
};
