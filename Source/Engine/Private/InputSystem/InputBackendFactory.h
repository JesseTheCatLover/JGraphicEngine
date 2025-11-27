//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/Memory/SmartPointers.h"
#include "IInputBackend.h"
#include "../Rendering/IPlatformSurface.h"

class InputBackendFactory
{
public:
    /**
     * @brief Create an input backend appropriate for the given platform surface.
     *
     * @param surface The active platform surface (window, etc.).
     * @return TUniquePtr<IInputBackend> or nullptr on failure.
     */
    static TUniquePtr<IInputBackend> MakeInputBackend(IPlatformSurface* surface);
};
