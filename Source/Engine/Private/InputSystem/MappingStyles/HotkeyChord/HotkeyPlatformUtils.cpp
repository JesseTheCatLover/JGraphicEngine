//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InputSystem/MappingStyles/HotkeyChord/HotkeyPlatformUtils.h"

EInputPlatform DetectInputPlatform()
{
#if defined(_WIN32) || defined(_WIN64)
    return EInputPlatform::Windows;
#elif defined(__APPLE__)
    return EInputPlatform::MacOS;
#elif defined(__linux__)
    return EInputPlatform::Linux;
#else
    // Safe default
    return EInputPlatform::Windows;
#endif
}