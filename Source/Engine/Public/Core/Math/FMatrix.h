// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>
#include "FVector3.h"
#include "FQuat.h"
#include "glm/matrix.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/quaternion.hpp"

/**
 * @struct FMatrix
 * @brief Represents a 4x4 matrix for 3D transformations.
 *
 * Wraps glm::mat4 internally and exposes an API for the Engine.
 */
struct FMatrix
{
private:
    glm::mat4 M{ 1.0f };

public:
    /** Default constructor. Initializes to identity. */
    FMatrix() = default;

    /** Constructs from a glm::mat4. */
    explicit FMatrix(const glm::mat4& mat) : M(mat) {}

    [[nodiscard]] glm::mat4 Get() const { return M; } // TODO: Maybe temp

    /** Matrix multiplication */
    FMatrix operator*(const FMatrix& other) const { return FMatrix(M * other.M); }
    FMatrix& operator*=(const FMatrix& other) { M *= other.M; return *this; }

    /** Comparison operators */
    bool operator==(const FMatrix& other) const { return M == other.M; }
    bool operator!=(const FMatrix& other) const { return M != other.M; }

    /** Returns the identity matrix */
    static FMatrix Identity() { return FMatrix(glm::mat4(1.0f)); }

    /** Creates a translation matrix from a vector */
    static FMatrix Translate(const FVector3& t) { return FMatrix(glm::translate(glm::mat4(1.0f), glm::vec3(t.x, t.y, t.z))); }

    /** Creates a scaling matrix from a vector */
    static FMatrix Scale(const FVector3& s) { return FMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(s.x, s.y, s.z))); }

    /** Creates a rotation matrix from an FQuat */
    static FMatrix Rotate(const FQuat& q) { return FMatrix(glm::toMat4(static_cast<glm::quat>(q))); }

    /** Determinant of the matrix */
    [[nodiscard]] float Determinant() const { return glm::determinant(M); }

    /** Returns a new inverse matrix */
    [[nodiscard]] FMatrix Inverse() const { return FMatrix(glm::inverse(M)); }

    /** Returns a new transposed matrix */
    [[nodiscard]] FMatrix Transpose() const { return FMatrix(glm::transpose(M)); }

    /** Inverts the matrix in-place */
    FMatrix& InvertSelf() { M = glm::inverse(M); return *this; }

    /** Transposes the matrix in-place */
    FMatrix& TransposeSelf() { M = glm::transpose(M); return *this; }

    /**
     * @brief Transforms a 3D point (applies translation)
     * @param point The point to transform
     * @return Transformed point
     */
    [[nodiscard]] FVector3 TransformPoint(const FVector3& point) const
    {
        float rx = M[0][0] * point.x + M[0][1] * point.y + M[0][2] * point.z + M[0][3];
        float ry = M[1][0] * point.x + M[1][1] * point.y + M[1][2] * point.z + M[1][3];
        float rz = M[2][0] * point.x + M[2][1] * point.y + M[2][2] * point.z + M[2][3];
        return {rx, ry, rz};
    }

    /**
     * @brief Transforms a 3D vector (ignores translation)
     * @param vec The vector to transform
     * @return Transformed vector
     */
    [[nodiscard]] FVector3 TransformVector(const FVector3& vec) const
    {
        float rx = M[0][0] * vec.x + M[0][1] * vec.y + M[0][2] * vec.z;
        float ry = M[1][0] * vec.x + M[1][1] * vec.y + M[1][2] * vec.z;
        float rz = M[2][0] * vec.x + M[2][1] * vec.y + M[2][2] * vec.z;
        return {rx, ry, rz};
    }

    [[nodiscard]] FEuler ToEuler() const
    {
        glm::vec3 euler = glm::eulerAngles(glm::quat_cast(M));
        return FEuler(euler.x, euler.y, euler.z);
    }

    static FMatrix MakeFromEuler(const FEuler& euler)
    {
        glm::mat4 m = glm::yawPitchRoll(euler.Yaw, euler.Pitch, euler.Roll);
        return FMatrix(m);
    }

    [[nodiscard]] FRotator ToRotator() const
    {
        return ToEuler().ToRotator();
    }

    static FMatrix MakeFromRotator(const FRotator& rotator)
    {
        return MakeFromEuler(FEuler::MakeFromRotator(rotator));
    }

    [[nodiscard]] FQuat ToQuat() const
    {
        return FQuat(glm::quat_cast(M));
    }

    static FMatrix MakeFromQuat(const FQuat& quat)
    {
        return FMatrix(glm::toMat4(quat.operator glm::quat()));
    }

    /** Converts to glm::mat4 for internal use */
    explicit operator glm::mat4() const { return M; }

    /** Returns a string representation of the matrix */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        for (int row = 0; row < 4; ++row)
        {
            ss << M[row][0] << " " << M[row][1] << " " << M[row][2] << " " << M[row][3] << "\n";
        }
        return ss.str();
    }
};
