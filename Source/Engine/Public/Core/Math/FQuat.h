// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <sstream>
#include <string>
#include "FEuler.h"
#include "glm/gtx/quaternion.hpp"

struct FVector3;
struct FMatrix4;
struct FRotator;

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
    explicit FQuat(const FVector3& axis, float angle);

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
    [[nodiscard]] FVector3 RotateVector(const FVector3& vector) const;

    /** Converts quaternion to rotation matrix */
    [[nodiscard]] FMatrix4 ToMatrix() const;

    [[nodiscard]] FRotator ToRotator() const;

    static FQuat MakeFromRotator(const FRotator& rotator);

    /**
     * @brief Converts this quaternion to Euler angles (radians).
     * @return Euler angles representing the same rotation.
     */
    [[nodiscard]] FEuler ToEuler() const;

    /**
     * @brief Sets this quaternion from Euler angles (radians).
     */
    static FQuat MakeFromEuler(const FEuler &euler);

    static FQuat MakeFromVector3(const FVector3& vector);

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
