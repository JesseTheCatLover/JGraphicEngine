//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/Math/FVector3.h"

struct FRay
{
    FVector3 Origin;
    FVector3 Direction; // Should be normalized
};

struct FRaycastHit
{
    bool      bHit        = false;
    int       ActorID     = -1;
    float     Distance    = 0.0f;
    FVector3  Position;
    FVector3  Normal;
};
