//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

enum class EInputPlatform : uint8_t
{
    Windows,
    Linux,
    MacOS
};

enum class EHotkeyPlatformMask : uint8_t
{
    Any     = 0,
    Windows = 1 << 0,
    Linux   = 1 << 1,
    MacOS   = 1 << 2
};

inline EHotkeyPlatformMask operator|(EHotkeyPlatformMask a, EHotkeyPlatformMask b)
{
    return static_cast<EHotkeyPlatformMask>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline EHotkeyPlatformMask operator&(EHotkeyPlatformMask a, EHotkeyPlatformMask b)
{
    return static_cast<EHotkeyPlatformMask>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool HasAnyFlag(EHotkeyPlatformMask value, EHotkeyPlatformMask flags)
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flags)) != 0;
}

inline EHotkeyPlatformMask ToPlatformMask(EInputPlatform p)
{
    switch (p)
    {
        case EInputPlatform::Windows: return EHotkeyPlatformMask::Windows;
        case EInputPlatform::Linux:   return EHotkeyPlatformMask::Linux;
        case EInputPlatform::MacOS:   return EHotkeyPlatformMask::MacOS;
        default:                      return EHotkeyPlatformMask::Any;
    }
}