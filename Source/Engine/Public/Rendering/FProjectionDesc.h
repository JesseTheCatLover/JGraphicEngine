//  Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "EProjectionType.h"

struct FProjectionDesc
{
    EProjectionType type = EProjectionType::Perspective;

    // Perspective
    float fovYDeg = 60.f;

    // Ortho (vertical half-size in *engine units*)
    float orthoHalfHeight = 10.f;

    // Shared
    float aspect = 16.f / 9.f;
    float nearP  = 0.1f;
    float farP   = 1000.f;
};
