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
    glm::vec3 euler = glm::eulerAngles(glm::quat_cast(M));
    return FEuler(euler.x, euler.y, euler.z);
}

FMatrix4 FMatrix4::MakeFromEuler(const FEuler& euler)
{
    glm::mat4 m = glm::yawPitchRoll(euler.Yaw, euler.Pitch, euler.Roll);
    return FMatrix4(m);
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
