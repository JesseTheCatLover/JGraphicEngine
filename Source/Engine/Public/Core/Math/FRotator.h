//  Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include "FVector3.h"
#include "FMath.h"
#include <sstream>
#include <string>

#include "FQuat.h"

/**
 * @struct FRotator
 * @brief Represents a user-facing rotation in degrees.
 *
 * Stores (Pitch=X, Yaw=Y, Roll=Z) in degrees.
 * This is a lightweight container intended for Gameplay API, UI, editor, or serialization purposes.
 * All internal math types use radians and can handle conversion of FRotator.
 */
struct FRotator
{
    float Pitch{0.0f};  ///< Rotation around X-axis (degrees)
    float Yaw{0.0f};    ///< Rotation around Y-axis (degrees)
    float Roll{0.0f};   ///< Rotation around Z-axis (degrees)

    /** Default constructor */
    FRotator() = default;

    /** Construct from components */
    constexpr FRotator(float pitch, float yaw, float roll)
        : Pitch(pitch), Yaw(yaw), Roll(roll) {}

    /** Construct from FVector3 (interpreted as degrees) */
    explicit FRotator(const FVector3& vec)
        : Pitch(vec.x), Yaw(vec.y), Roll(vec.z) {}

    /** Convert to FVector3 (degrees) */
    [[nodiscard]] FVector3 ToVector3() const { return {Pitch, Yaw, Roll}; }

    /** Convert to FEuler (radians) */
    [[nodiscard]] FEuler ToEuler() const
    {
        return {
            FMath::Radians(Pitch),
            FMath::Radians(Yaw),
            FMath::Radians(Roll)
        };
    }

    /** Convert to quaternion (radians) */
    [[nodiscard]] FQuat ToQuat() const
    {
        return FQuat::MakeFromEuler(ToEuler());
    }

    /** String representation */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << "FRotator(Pitch=" << Pitch << ", Yaw=" << Yaw << ", Roll=" << Roll << ")";
        return ss.str();
    }

    /** Equality operators */
    bool operator==(const FRotator& other) const
    {
        return Pitch == other.Pitch && Yaw == other.Yaw && Roll == other.Roll;
    }

    bool operator!=(const FRotator& other) const { return !(*this == other); }
};
