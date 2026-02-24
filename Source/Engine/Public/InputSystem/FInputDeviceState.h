//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <unordered_map>
#include "InputSystem/EPhysicalInput.h"

enum class EInputDeviceType : uint8_t
{
    Keyboard,
    Mouse,
    Gamepad,
    Touchpad
};

struct FInputDeviceState
{
    EInputDeviceType type;
    int index; // e.g. which gamepad, keyboard, ...

    // Unified per-physical-input value:
    //  - Keyboard_W, Keyboard_Space  -> 0 or 1
    //  - Mouse_AxisX            -> dx this frame
    //  - Mouse_WheelY                -> wheel delta
    //  - Gamepad_ButtonSouth         -> 0 or 1
    //  - Gamepad_LeftStickX          -> -1..1
    std::unordered_map<EPhysicalInput, float> values;
};