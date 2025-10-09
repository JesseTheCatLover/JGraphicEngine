// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include "FVector3.h"
#include "glm/gtc/quaternion.hpp"
#include <sstream>
#include <string>

#include "FMath.h"
#include "FRotator.h"

struct FQuat;
/**
 * @struct FEuler
 * @brief Represents rotation in Euler angles (pitch, yaw, roll), in radians.
 *
 * Provides conversions to and from FQuat and FVector3.
 * Internally uses radians for consistency with internal calculations.
 *
 * Pitch (X) = rotation around X-axis
 * Yaw   (Y) = rotation around Y-axis
 * Roll  (Z) = rotation around Z-axis
 */
struct FEuler
{
    float Pitch{0.0f};  ///< Rotation around X-axis (radians)
    float Yaw{0.0f};    ///< Rotation around Y-axis (radians)
    float Roll{0.0f};   ///< Rotation around Z-axis (radians)

    /** Default constructor (identity rotation). */
    FEuler() = default;

    /** Constructs from individual components. */
    constexpr FEuler(float pitch, float yaw, float roll)
        : Pitch(pitch), Yaw(yaw), Roll(roll) {}

    /** Constructs from a vector (X=Pitch, Y=Yaw, Z=Roll). */
    explicit FEuler(const FVector3& vector)
        : Pitch(vector.x), Yaw(vector.y), Roll(vector.z) {}

    /** Converts to a FVector3 in radians (X=Pitch, Y=Yaw, Z=Roll). */
    [[nodiscard]] FVector3 ToVector3() const
    {
        return {Pitch, Yaw, Roll};
    }

    /** Creates FEuler from a vector in radians (X=Pitch, Y=Yaw, Z=Roll). */
    static FEuler MakeFromVector3(const FVector3& vector)
    {
        return {vector.x, vector.y, vector.z};
    }

    /** Converts to FRotator in degrees. */
    [[nodiscard]] FRotator ToRotator() const
    {
        return FRotator{
            FMath::Degrees(Pitch),
            FMath::Degrees(Yaw),
            FMath::Degrees(Roll)
        };
    }

    static FEuler MakeFromRotator(const FRotator& rotator)
    {
        return FEuler{ FMath::Radians(rotator.Pitch),
                       FMath::Radians(rotator.Yaw),
                       FMath::Radians(rotator.Roll) };
    }

    /** Converts to quaternion. */
    [[nodiscard]] FQuat ToQuat() const
    {
        glm::quat q = glm::quat(glm::vec3(Pitch, Yaw, Roll));
        return FQuat(q);
    }

    /** Creates FEuler from a quaternion. */
    static FEuler MakeFromQuat(const FQuat& quat)
    {
        glm::vec3 euler = glm::eulerAngles(quat.operator glm::quat());
        return {euler.x, euler.y, euler.z};
    }

    /** String representation in radians. */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << "Euler(Pitch=" << Pitch << ", Yaw=" << Yaw << ", Roll=" << Roll << ")";
        return ss.str();
    }
};
