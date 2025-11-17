//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Math/FQuat.h"

#include "Core/Math/FEuler.h"
#include "Core/Math/FMatrix4.h"
#include "Core/Math/FRotator.h"

/** Constructs quaternion from axis-angle rotation. Angle in radians. */
FQuat::FQuat(const FVector3& axis, float angle)
{
    glm::vec3 glmAxis(axis.x, axis.y, axis.z);
    Q = glm::angleAxis(angle, glmAxis);
}

FVector3 FQuat::RotateVector(const FVector3& vector) const
{
    glm::vec3 rotated = Q * glm::vec3(vector.x, vector.y, vector.z);
    return {rotated.x, rotated.y, rotated.z};
}

FMatrix4 FQuat::ToMatrix() const
{
    return FMatrix4(glm::toMat4(Q));
}

[[nodiscard]] FRotator FQuat::ToRotator() const
{
    return FEuler::MakeFromQuat(*this).ToRotator();
}

FQuat FQuat::MakeFromRotator(const FRotator& rotator)
{
    return FEuler::MakeFromRotator(rotator).ToQuat();
}

FEuler FQuat::ToEuler() const
{
    return FEuler::MakeFromQuat(*this);
}

FQuat FQuat::MakeFromEuler(const FEuler &euler)
{
    return euler.ToQuat();
}

FQuat FQuat::MakeFromVector3(const FVector3& vector)
{
    return MakeFromEuler(FEuler::MakeFromVector3(vector));
}
