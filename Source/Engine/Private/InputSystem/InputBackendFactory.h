//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/Memory/SmartPointers.h"
#include "IInputBackend.h"
#include "Rendering/IPlatformWindow.h"

class InputBackendFactory
{
public:
    /**
     * @brief Create an input backend appropriate for the given platform window.
     *
     * @param window The active platform window.
     * @return TUniquePtr<IInputBackend> or nullptr on failure.
     */
    static TUniquePtr<IInputBackend> MakeInputBackend(IPlatformWindow* window);
};
