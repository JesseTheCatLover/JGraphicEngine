//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

struct FActionStateBool
{
    bool pressed = false;
    bool released = false;
    bool held = false;
};

struct FActionStateAxis1D
{
    float value = 0.f;
    float delta = 0.f;
};

struct FActionStateAxis2D
{
    float x = 0.f;
    float y = 0.f;
    float dx = 0.f;
    float dy = 0.f;
};
