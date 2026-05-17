// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

enum class EProjectLaunchAction : uint8_t
{
    Cancel = 0,
    OpenExisting,
    CreateNew,
    None
};