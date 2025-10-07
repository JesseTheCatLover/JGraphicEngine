// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>

#include "FMath.h"
#include "glm/gtx/quaternion.hpp"

/**
* @struct FQuat
 * @brief Represents a quaternion for 3D rotations.
 *
 * Internally wraps glm::quat, which stores quaternions in the order (w, x, y, z):
 * - w: scalar part
 * - x, y, z: vector part
 *
 * Provides constructors, arithmetic, normalization, inversion, vector rotation,
 * and conversion to rotation matrices.

 * Wraps glm::quat internally and exposes an API for the Engine.
 */
struct FQuat
{
private:
    glm::quat Q{1, 0, 0, 0}; // Default identity quaternion (w, x, y, z)

public:
    [[nodiscard]] float x() const { return Q.x; }
    [[nodiscard]] float y() const { return Q.y; }
    [[nodiscard]] float z() const { return Q.z; }
    [[nodiscard]] float w() const { return Q.w; }

    /** Default constructor. Identity quaternion. */
    FQuat() = default;

    /** Constructs quaternion from components. */
    constexpr FQuat(float InX, float InY, float InZ, float InW) : Q(InW, InX, InY, InZ) {}

    /** Constructs quaternion from axis-angle rotation. Angle in radians. */
    explicit FQuat(const FVector3& Axis, float Angle)
    {
        glm::vec3 glmAxis(Axis.x, Axis.y, Axis.z);
        Q = glm::angleAxis(Angle, glmAxis);
    }

    /** Constructs from glm::quat directly. */
    explicit FQuat(const glm::quat& InQuat) : Q(InQuat) {}

    /** Quaternion multiplication */
    FQuat operator*(const FQuat& Other) const { return FQuat(Q * Other.Q); }
    FQuat& operator*=(const FQuat& Other) { Q *= Other.Q; return *this; }

    /** Checks if two quaternions are equal */
    bool operator==(const FQuat& Other) const
    { return Q == Other.Q; }

    /** Not equal operator */
    bool operator!=(const FQuat& Other) const { return !(*this == Other); }

    /** Returns the conjugate of the quaternion */
    [[nodiscard]] FQuat Conjugate() const { return FQuat(glm::conjugate(Q)); }

    /** Returns the inverse of the quaternion */
    [[nodiscard]] FQuat Inverse() const { return FQuat(glm::inverse(Q)); }

    /** Normalizes the quaternion */
    [[nodiscard]] FQuat Normalized() const { return FQuat(glm::normalize(Q)); }

    /** Rotates a vector by this quaternion */
    [[nodiscard]] FVector3 RotateVector(const FVector3& Vec) const
    {
        glm::vec3 rotated = Q * glm::vec3(Vec.x, Vec.y, Vec.z);
        return {rotated.x, rotated.y, rotated.z};
    }

    /** Converts quaternion to rotation matrix */
    [[nodiscard]] FMatrix ToMatrix() const
    {
        return FMatrix(glm::toMat4(Q));
    }

    /**
     * @brief Converts this quaternion to Euler angles (radians).
     * @return Euler angles representing the same rotation.
     */
    [[nodiscard]] FEuler ToEuler() const
    {
        glm::vec3 eulerRad = glm::eulerAngles(Q); // GLM returns radians
        return {eulerRad.x, eulerRad.y, eulerRad.z};
    }

    /**
     * @brief Converts this quaternion to Euler angles in degrees.
     * @return Euler angles in degrees.
     */
    [[nodiscard]] FEuler ToEulerDegrees() const
    {
        FEuler eulerRad = ToEuler();
        FEuler eulerDeg;
        eulerDeg.SetPitchDegrees(FMath::Degrees(eulerRad.Pitch));
        eulerDeg.SetYawDegrees(FMath::Degrees(eulerRad.Yaw));
        eulerDeg.SetRollDegrees(FMath::Degrees(eulerRad.Roll));
        return eulerDeg;
    }

    /**
     * @brief Sets this quaternion from Euler angles (radians).
     */
    void FromEuler(const FEuler& Euler)
    {
        Q = glm::quat(glm::vec3(Euler.Pitch, Euler.Yaw, Euler.Roll));
    }

    /**
     * @brief Sets this quaternion from Euler angles in degrees.
     */
    void FromEulerDegrees(const FEuler& EulerDeg)
    {
        FromEuler(FEuler(FMath::Radians(EulerDeg.GetPitchDegrees()),
                         FMath::Radians(EulerDeg.GetYawDegrees()),
                         FMath::Radians(EulerDeg.GetRollDegrees())));
    }

    /** Converts to glm::quat for internal use */
    explicit operator glm::quat() const { return Q; }

    /** String representation */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << "Quat(" << x() << ", " << y() << ", " << z() << ", " << w() << ")";
        return ss.str();
    }
};
