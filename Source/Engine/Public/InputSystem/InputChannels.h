//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>

enum class EInputChannelType : uint8_t
{
    Bool,
    Axis1D,
    Axis2D
};

using InputChannelHandle = uint32_t;

struct FInputChannelDesc
{
    InputChannelHandle handle;
    std::string name; // "Jump", "Look", esc..
    EInputChannelType type;
};