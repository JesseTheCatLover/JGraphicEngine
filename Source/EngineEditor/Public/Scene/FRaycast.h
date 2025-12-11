//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/Math/FVector3.h"

struct FRay
{
    FVector3 origin;
    FVector3 direction; // Should be normalized
};

struct FRaycastHit
{
    bool bHit = false;
    uint64_t actorID = -1;
    float distance = 0.0f;
    FVector3 position;
    FVector3 normal;
};
