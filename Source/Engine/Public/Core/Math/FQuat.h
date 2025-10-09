// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>

#include "FEuler.h"
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
    constexpr FQuat(float x, float y, float z, float w) : Q(w, x, y, z) {}

    /** Constructs quaternion from axis-angle rotation. Angle in radians. */
    explicit FQuat(const FVector3& axis, float angle)
    {
        glm::vec3 glmAxis(axis.x, axis.y, axis.z);
        Q = glm::angleAxis(angle, glmAxis);
    }

    /** Constructs from glm::quat directly. */
    explicit FQuat(const glm::quat& quat) : Q(quat) {}

    /** Quaternion multiplication */
    FQuat operator*(const FQuat& other) const { return FQuat(Q * other.Q); }
    FQuat& operator*=(const FQuat& other) { Q *= other.Q; return *this; }

    /** Checks if two quaternions are equal */
    bool operator==(const FQuat& other) const
    { return Q == other.Q; }

    /** Not equal operator */
    bool operator!=(const FQuat& other) const { return !(*this == other); }

    /** Returns the conjugate of the quaternion */
    [[nodiscard]] FQuat Conjugate() const { return FQuat(glm::conjugate(Q)); }

    /** Returns the inverse of the quaternion */
    [[nodiscard]] FQuat Inverse() const { return FQuat(glm::inverse(Q)); }

    /** Normalizes the quaternion */
    [[nodiscard]] FQuat Normalized() const { return FQuat(glm::normalize(Q)); }

    /** Rotates a vector by this quaternion */
    [[nodiscard]] FVector3 RotateVector(const FVector3& vector) const
    {
        glm::vec3 rotated = Q * glm::vec3(vector.x, vector.y, vector.z);
        return {rotated.x, rotated.y, rotated.z};
    }

    /** Converts quaternion to rotation matrix */
    [[nodiscard]] FMatrix ToMatrix() const
    {
        return FMatrix(glm::toMat4(Q));
    }

    [[nodiscard]] FRotator ToRotator() const
    {
        return FEuler::MakeFromQuat(*this).ToRotator();
    }

    static FQuat MakeFromRotator(const FRotator& rotator)
    {
        return FEuler::MakeFromRotator(rotator).ToQuat();
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
     * @brief Sets this quaternion from Euler angles (radians).
     */
    void MakeFromEuler(const FEuler& euler)
    {
        Q = glm::quat(glm::vec3(euler.Pitch, euler.Yaw, euler.Roll));
    }

    FQuat MakeFromVector3(const FVector3& vector)
    {
        return MakeFromEuler(vector);
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
