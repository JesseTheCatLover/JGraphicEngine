//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
#pragma once

#include "FVector3.h"
#include <sstream>
#include <string>

struct FQuat;

/**
 * @struct FRotator
 * @brief Represents a user-facing rotation in degrees.
 *
 * Stores (Pitch, Yaw, Roll) in degrees using the engine's convention (X forward, Y right, Z up):
 * - Roll  = rotation around X-axis (forward)
 * - Pitch = rotation around Y-axis (right)
 * - Yaw   = rotation around Z-axis (up)
 * This is a lightweight container intended for Gameplay API, UI, editor, or serialization purposes.
 * All internal math types use radians and can handle conversion of FRotator.
 */
struct FRotator
{
    float Pitch{0.0f};  ///< Rotation around Y-axis (degrees)
    float Yaw{0.0f};    ///< Rotation around Z-axis (degrees)
    float Roll{0.0f};   ///< Rotation around X-axis (degrees)

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

    /** Convert to FEuler (radians). */
    [[nodiscard]] FEuler ToEuler() const;

    /** Convert to quaternion. */
    [[nodiscard]] FQuat ToQuat() const;

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

    constexpr FRotator operator+(const FRotator& rot) const { return {Pitch + rot.Pitch, Yaw + rot.Yaw, Roll + rot.Roll}; }
    constexpr FRotator operator-(const FRotator& rot) const { return {Pitch - rot.Pitch, Yaw - rot.Yaw, Roll - rot.Roll}; }

    constexpr FRotator operator*(const float& scale) const { return {Pitch * scale, Yaw * scale, Roll * scale}; }
};