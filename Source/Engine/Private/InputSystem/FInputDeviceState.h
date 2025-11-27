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
    int index; // e.g. which gamepad, keyboard, ...
    std::vector<float> buttons;  // 0..1 pressed amount
    std::vector<float> axes; // -1..1 / 0..1 etc.
};