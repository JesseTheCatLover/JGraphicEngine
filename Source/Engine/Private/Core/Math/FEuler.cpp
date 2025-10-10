//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Math/FEuler.h"
#include "Core/Math/FMath.h"

/** Converts to FRotator in degrees. */
FRotator FEuler::ToRotator() const
{
    return FRotator{
        FMath::Degrees(Pitch),
        FMath::Degrees(Yaw),
        FMath::Degrees(Roll)
    };
}

FEuler FEuler::MakeFromRotator(const FRotator& rotator)
{
    return FEuler{
        FMath::Radians(rotator.Pitch),
        FMath::Radians(rotator.Yaw),
        FMath::Radians(rotator.Roll)
    };
}

/** Converts to quaternion. */
FQuat FEuler::ToQuat() const
{
    glm::quat q = glm::quat(glm::vec3(Pitch, Yaw, Roll));
    return FQuat(q);
}

/** Creates FEuler from a quaternion. */
FEuler FEuler::MakeFromQuat(const FQuat& quat)
{
    glm::vec3 euler = glm::eulerAngles(quat.operator glm::quat());
    return {euler.x, euler.y, euler.z};
}
