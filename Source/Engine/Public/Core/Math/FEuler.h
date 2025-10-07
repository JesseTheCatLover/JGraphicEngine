// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include "FVector3.h"
#include "glm/gtc/quaternion.hpp"
#include <sstream>
#include <string>
#include "FMath.h"

struct FQuat;
/**
 * @struct FEuler
 * @brief Represents rotation in Euler angles (pitch, yaw, roll), in radians.
 *
 * Provides conversions to and from FQuat and FVector3.
 * Internally uses radians for consistency with glm.
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
    constexpr FEuler(float InPitch, float InYaw, float InRoll)
        : Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}

    /** Constructs from a vector (X=Pitch, Y=Yaw, Z=Roll). */
    explicit FEuler(const FVector3& Vec)
        : Pitch(Vec.x), Yaw(Vec.y), Roll(Vec.z) {}

    /**
    * @brief Gets Pitch in degrees.
     */
    [[nodiscard]] float GetPitchDegrees() const { return FMath::Degrees(Pitch); }

    /**
     * @brief Gets Yaw in degrees.
     */
    [[nodiscard]] float GetYawDegrees() const { return FMath::Degrees(Yaw); }

    /**
     * @brief Gets Roll in degrees.
     */
    [[nodiscard]] float GetRollDegrees() const { return FMath::Degrees(Roll); }

    /**
     * @brief Sets Pitch from degrees.
     */
    void SetPitchDegrees(float Degrees) { Pitch = FMath::Radians(Degrees); }

    /**
     * @brief Sets Yaw from degrees.
     */
    void SetYawDegrees(float Degrees) { Yaw = FMath::Radians(Degrees); }

    /**
     * @brief Sets Roll from degrees.
     */
    void SetRollDegrees(float Degrees) { Roll = FMath::Radians(Degrees); }

    /**
     * @brief Returns the Euler angles as a vector in degrees.
     */
    [[nodiscard]] FVector3 ToDegreesVector() const
    {
        return {GetPitchDegrees(), GetYawDegrees(), GetRollDegrees()};
    }

    /**
     * @brief Creates FEuler from a vector in degrees.
     */
    static FEuler FromDegreesVector(const FVector3& DegreesVec)
    {
        return {FMath::Radians(DegreesVec.x), FMath::Radians(DegreesVec.y), FMath::Radians(DegreesVec.z)};
    }

    /** Converts to a FVector3 (X=Pitch, Y=Yaw, Z=Roll). */
    [[nodiscard]] FVector3 ToVector3() const
    {
        return {Pitch, Yaw, Roll};
    }

    /** Creates FEuler from a vector (X=Pitch, Y=Yaw, Z=Roll). */
    static FEuler FromVector3(const FVector3& Vec)
    {
        return {Vec.x, Vec.y, Vec.z};
    }

    /** Converts to quaternion. */
    [[nodiscard]] FQuat ToQuat() const
    {
        glm::quat q = glm::quat(glm::vec3(Pitch, Yaw, Roll));
        return FQuat(q);
    }

    /** Creates FEuler from a quaternion. */
    static FEuler FromQuat(const FQuat& Quat)
    {
        glm::vec3 euler = glm::eulerAngles(Quat.operator glm::quat());
        return {euler.x, euler.y, euler.z};
    }

    /** String representation in radians. */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << "Euler(Pitch=" << Pitch << ", Yaw=" << Yaw << ", Roll=" << Roll << ")";
        return ss.str();
    }

    /**
     * @brief Returns a string representation of the Euler angles in degrees.
    */
    [[nodiscard]] std::string ToDegreesString() const
    {
        std::ostringstream ss;
        ss << "Euler(Pitch=" << GetPitchDegrees()
           << "°, Yaw=" << GetYawDegrees()
           << "°, Roll=" << GetRollDegrees() << "°)";
        return ss.str();
    }
};
