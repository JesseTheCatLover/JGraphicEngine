//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>

#include "InputSystem/FRawInputEvent.h"

class IInputBackend
{
public:
    virtual ~IInputBackend() = default;

    // Collects all input events since last frame
    virtual void FetchEvents(std::vector<FRawInputEvent>& outEvents) = 0;

    virtual void GetMousePosition(float& outX, float& outY) = 0;
};
