//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Math/FMatrix.h"
#include "Core/Math/FEuler.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FRotator.h"

#include "glm/gtx/euler_angles.hpp"

FMatrix FMatrix::Rotate(const FQuat &q)
{
    return FMatrix(glm::toMat4(static_cast<glm::quat>(q)));
}

[[nodiscard]] FEuler FMatrix::ToEuler() const
{
    glm::vec3 euler = glm::eulerAngles(glm::quat_cast(M));
    return FEuler(euler.x, euler.y, euler.z);
}

FMatrix FMatrix::MakeFromEuler(const FEuler& euler)
{
    glm::mat4 m = glm::yawPitchRoll(euler.Yaw, euler.Pitch, euler.Roll);
    return FMatrix(m);
}

[[nodiscard]] FRotator FMatrix::ToRotator() const
{
    return ToEuler().ToRotator();
}

FMatrix FMatrix::MakeFromRotator(const FRotator& rotator)
{
    return MakeFromEuler(FEuler::MakeFromRotator(rotator));
}

[[nodiscard]] FQuat FMatrix::ToQuat() const
{
    return FQuat(glm::quat_cast(M));
}

FMatrix FMatrix::MakeFromQuat(const FQuat& quat)
{
    return FMatrix(glm::toMat4(quat.operator glm::quat()));
}
