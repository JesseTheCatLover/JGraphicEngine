//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Math/FRotator.h"
#include "Core/Math/FMath.h"


FEuler FRotator::ToEuler() const
{
    return {
        FMath::Radians(pitch),
        FMath::Radians(yaw),
        FMath::Radians(roll)
    };
}

FQuat FRotator::ToQuat() const
{
    return FQuat::MakeFromEuler(ToEuler());
}