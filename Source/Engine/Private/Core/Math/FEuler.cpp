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
    const FVector3 axisRoll  (1.0f, 0.0f, 0.0f); // X (forward)
    const FVector3 axisPitch (0.0f, 1.0f, 0.0f); // Y (right)
    const FVector3 axisYaw   (0.0f, 0.0f, 1.0f); // Z (up)

    FQuat qRoll (axisRoll,  Roll);
    FQuat qPitch(axisPitch, Pitch);
    FQuat qYaw  (axisYaw,   Yaw);

    return qYaw * qPitch * qRoll;
}

/**
 * Creates FEuler from a quaternion, inverse of ToQuat().
 *
 * Uses ZYX Tait–Bryan decomposition of the rotation matrix:
 * R = Rz(Yaw) * Ry(Pitch) * Rx(Roll)
 */
FEuler FEuler::MakeFromQuat(const FQuat& q)
{
    // Unpack
    const float x = q.x();
    const float y = q.y();
    const float z = q.z();
    const float w = q.w();

    // Precompute products
    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;

    const float xy = x * y;
    const float xz = x * z;
    const float yz = y * z;

    const float wx = w * x;
    const float wy = w * y;
    const float wz = w * z;

    // Build only the elements we need from the 3x3 rotation matrix
    const float r00 = 1.0f - 2.0f * (yy + zz);
    const float r10 = 2.0f * (xy + wz);
    const float r20 = 2.0f * (xz - wy);
    const float r21 = 2.0f * (yz + wx);
    const float r22 = 1.0f - 2.0f * (xx + yy);

    const float r01 = 2.0f * (xy - wz);   // needed only for gimbal lock
    const float r11 = 1.0f - 2.0f * (xx + zz);

    FEuler out{};
    constexpr float eps = 1e-6f;

    // Check for standard case vs gimbal lock
    if (FMath::Abs(r20) < 1.0f - eps)
    {
        // Standard ZYX extraction
        out.Pitch = std::asin(-r20);
        out.Yaw   = std::atan2(r10, r00);
        out.Roll  = std::atan2(r21, r22);
    }
    else
    {
        // Gimbal lock: Pitch is +/- 90 degrees
        out.Pitch = (r20 <= 0.0f)
                        ? FMath::Radians(90.0f)
                        : FMath::Radians(-90.0f);

        out.Roll = 0.0f;
        out.Yaw  = std::atan2(-r01, r11);
    }

    return out;
}