//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Math/FVector3.h"
#include "Core/Math/FEuler.h"
#include "Core/Math/FRotator.h"

FRotator FVector3::ToRotator() const
{
    return FRotator{x, y, z};
}

FEuler FVector3::ToEuler() const
{
    return FEuler{x, y, z};
}