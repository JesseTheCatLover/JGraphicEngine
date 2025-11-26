//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <vector>

enum class EInputDeviceType : uint8_t
{
    Keyboard,
    Mouse,
    Gamepad
};

struct FInputDeviceState
{
    EInputDeviceType type;
    int index;
    std::vector<float> buttons;  // 0 or 1 (or trigger value)
    std::vector<float> axes; // -1..1 / 0..1 etc.
};