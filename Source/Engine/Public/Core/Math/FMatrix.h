// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>
#include "FVector3.h"
#include "FQuat.h"
#include "glm/matrix.hpp"
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
    explicit FMatrix(const glm::mat4& InMat) : M(InMat) {}

    [[nodiscard]] glm::mat4 Get() const { return M; } // TODO: Maybe temp

    /** Matrix multiplication */
    FMatrix operator*(const FMatrix& Other) const { return FMatrix(M * Other.M); }
    FMatrix& operator*=(const FMatrix& Other) { M *= Other.M; return *this; }

    /** Comparison operators */
    bool operator==(const FMatrix& Other) const { return M == Other.M; }
    bool operator!=(const FMatrix& Other) const { return M != Other.M; }

    /** Returns the identity matrix */
    static FMatrix Identity() { return FMatrix(glm::mat4(1.0f)); }

    /** Creates a translation matrix from a vector */
    static FMatrix Translate(const FVector3& T) { return FMatrix(glm::translate(glm::mat4(1.0f), glm::vec3(T.x, T.y, T.z))); }

    /** Creates a scaling matrix from a vector */
    static FMatrix Scale(const FVector3& S) { return FMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(S.x, S.y, S.z))); }

    /** Creates a rotation matrix from an FQuat */
    static FMatrix Rotate(const FQuat& Q) { return FMatrix(glm::toMat4(static_cast<glm::quat>(Q))); }

    /** Determinant of the matrix */
    float Determinant() const { return glm::determinant(M); }

    /** Returns a new inverse matrix */
    FMatrix Inverse() const { return FMatrix(glm::inverse(M)); }

    /** Returns a new transposed matrix */
    FMatrix Transpose() const { return FMatrix(glm::transpose(M)); }

    /** Inverts the matrix in-place */
    FMatrix& InvertSelf() { M = glm::inverse(M); return *this; }

    /** Transposes the matrix in-place */
    FMatrix& TransposeSelf() { M = glm::transpose(M); return *this; }

    /**
     * @brief Transforms a 3D point (applies translation)
     * @param Point The point to transform
     * @return Transformed point
     */
    FVector3 TransformPoint(const FVector3& Point) const
    {
        float rx = M[0][0] * Point.x + M[0][1] * Point.y + M[0][2] * Point.z + M[0][3];
        float ry = M[1][0] * Point.x + M[1][1] * Point.y + M[1][2] * Point.z + M[1][3];
        float rz = M[2][0] * Point.x + M[2][1] * Point.y + M[2][2] * Point.z + M[2][3];
        return FVector3(rx, ry, rz);
    }

    /**
     * @brief Transforms a 3D vector (ignores translation)
     * @param Vec The vector to transform
     * @return Transformed vector
     */
    [[nodiscard]] FVector3 TransformVector(const FVector3& Vec) const
    {
        float rx = M[0][0] * Vec.x + M[0][1] * Vec.y + M[0][2] * Vec.z;
        float ry = M[1][0] * Vec.x + M[1][1] * Vec.y + M[1][2] * Vec.z;
        float rz = M[2][0] * Vec.x + M[2][1] * Vec.y + M[2][2] * Vec.z;
        return {rx, ry, rz};
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
