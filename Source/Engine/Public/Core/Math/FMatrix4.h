// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <sstream>
#include <string>
#include "FVector3.h"
#include "glm/matrix.hpp"
#include "glm/gtc/type_ptr.inl"
#include "glm/gtx/quaternion.hpp"

struct FQuat;
struct FEuler;
struct FRotator;

/**
 * @struct FMatrix4
 * @brief Represents a 4x4 matrix for 3D transformations.
 *
 * Wraps glm::mat4 internally and exposes an API for the Engine.
 */
struct FMatrix4
{
private:
    glm::mat4 M{ 1.0f };

public:
    /** Default constructor. Initializes to identity. */
    FMatrix4() = default;

    /** Constructs from a glm::mat4. */
    explicit FMatrix4(const glm::mat4& mat) : M(mat) {}

    [[nodiscard]] const float* GetValue() const { return glm::value_ptr(M); }
    [[nodiscard]] glm::mat4 GetMat4() const { return M; }
    [[nodiscard]] FVector3 GetTranslation() const
    {
        return FVector3(M[3][0], M[3][1], M[3][2]);
    }

    /** Matrix multiplication */
    FMatrix4 operator*(const FMatrix4& other) const { return FMatrix4(M * other.M); }
    FMatrix4& operator*=(const FMatrix4& other) { M *= other.M; return *this; }

    /** Comparison operators */
    bool operator==(const FMatrix4& other) const { return M == other.M; }
    bool operator!=(const FMatrix4& other) const { return M != other.M; }

    /** Returns the identity matrix */
    static FMatrix4 Identity() { return FMatrix4(glm::mat4(1.0f)); }

    /** Creates a translation matrix from a vector */
    static FMatrix4 Translate(const FVector3& t) { return FMatrix4(glm::translate(glm::mat4(1.0f), glm::vec3(t.x, t.y, t.z))); }

    /** Creates a scaling matrix from a vector */
    static FMatrix4 Scale(const FVector3& s) { return FMatrix4(glm::scale(glm::mat4(1.0f), glm::vec3(s.x, s.y, s.z))); }

    /** Creates a rotation matrix from an FQuat */
    static FMatrix4 Rotate(const FQuat & q);

    /** Determinant of the matrix */
    [[nodiscard]] float Determinant() const { return glm::determinant(M); }

    /** Returns a new inverse matrix */
    [[nodiscard]] FMatrix4 Inverse() const { return FMatrix4(glm::inverse(M)); }

    /** Returns a new transposed matrix */
    [[nodiscard]] FMatrix4 Transpose() const { return FMatrix4(glm::transpose(M)); }

    /** Inverts the matrix in-place */
    FMatrix4& InvertSelf() { M = glm::inverse(M); return *this; }

    /** Transposes the matrix in-place */
    FMatrix4& TransposeSelf() { M = glm::transpose(M); return *this; }

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

    [[nodiscard]] FEuler ToEuler() const;

    static FMatrix4 MakeFromEuler(const FEuler& euler);

    [[nodiscard]] FRotator ToRotator() const;

    static FMatrix4 MakeFromRotator(const FRotator& rotator);

    [[nodiscard]] FQuat ToQuat() const;

    static FMatrix4 MakeFromQuat(const FQuat& quat);

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
