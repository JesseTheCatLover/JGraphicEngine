//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Math/FMatrix4.h"
#include "Core/Math/FEuler.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FRotator.h"

#include "glm/gtx/euler_angles.hpp"

FMatrix4 FMatrix4::Rotate(const FQuat &q)
{
    return FMatrix4(glm::toMat4(static_cast<glm::quat>(q)));
}

[[nodiscard]] FEuler FMatrix4::ToEuler() const
{
    glm::quat q = glm::quat_cast(M);
    return FEuler::MakeFromQuat(FQuat(q));
}

FMatrix4 FMatrix4::MakeFromEuler(const FEuler& euler)
{
    FQuat q = euler.ToQuat();
    return q.ToMatrix();
}

[[nodiscard]] FRotator FMatrix4::ToRotator() const
{
    return ToEuler().ToRotator();
}

FMatrix4 FMatrix4::MakeFromRotator(const FRotator& rotator)
{
    return MakeFromEuler(FEuler::MakeFromRotator(rotator));
}

[[nodiscard]] FQuat FMatrix4::ToQuat() const
{
    return FQuat(glm::quat_cast(M));
}

FMatrix4 FMatrix4::MakeFromQuat(const FQuat& quat)
{
    return FMatrix4(glm::toMat4(quat.operator glm::quat()));
}
