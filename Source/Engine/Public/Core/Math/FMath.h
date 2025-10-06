//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "FVector2.h"
#include "FVector3.h"
#include "FVector4.h"
#include "FMatrix.h"
#include "FTransform.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

/**
 * @namespace FMath
 * @brief A math utility namespace for the engine. Provides vector, matrix, transform,
 * scalar, and angle functions fully wrapped for FVector, FMatrix, and FTransform types.
 */
namespace FMath
{
    /////////////////////
    // Angle Utilities
    /////////////////////

    /**
     * @brief Converts degrees to radians.
     * @param Degrees Angle in degrees.
     * @return Angle in radians.
     */
    inline float Radians(float Degrees) { return glm::radians(Degrees); }

    /**
     * @brief Converts radians to degrees.
     * @param Radians Angle in radians.
     * @return Angle in degrees.
     */
    inline float Degrees(float Radians) { return glm::degrees(Radians); }

    /////////////////////
    // Scalar Utilities
    /////////////////////

    /**
     * @brief Clamps a value between a minimum and maximum.
     * @param Value Input value.
     * @param Min Minimum bound.
     * @param Max Maximum bound.
     * @return Clamped value.
     */
    inline float Clamp(float Value, float Min, float Max) { return glm::clamp(Value, Min, Max); }

    /**
     * @brief Linearly interpolates between two values.
     * @param A Start value.
     * @param B End value.
     * @param Alpha Interpolation factor [0,1].
     * @return Interpolated value.
     */
    inline float Lerp(float A, float B, float Alpha) { return glm::mix(A, B, Alpha); }

    /**
     * @brief Returns the maximum of two values.
     * @param A First value.
     * @param B Second value.
     * @return Maximum value.
     */
    inline float Max(float A, float B) { return glm::max(A, B); }

    /**
     * @brief Returns the minimum of two values.
     * @param A First value.
     * @param B Second value.
     * @return Minimum value.
     */
    inline float Min(float A, float B) { return glm::min(A, B); }

    /**
     * @brief Returns the absolute value.
     * @param Value Input value.
     * @return Absolute value.
     */
    inline float Abs(float Value) { return glm::abs(Value); }

    /**
     * @brief Returns the sign of a value.
     * @param Value Input value.
     * @return 1.0 if positive, -1.0 if negative.
     */
    inline float Sign(float Value) { return (Value >= 0) ? 1.0f : -1.0f; }

    /////////////////////
    // Vector Utilities
    /////////////////////

    /**
    * @brief Returns the length (magnitude) of a vector.
    * @param V Input vector.
    * @return Length of the vector.
    */
    template<typename VecType>
    inline float Length(const VecType &V) { return V.Length(); }

    /**
     * @brief Returns the normalized (unit) vector.
     * @param V Input vector.
     * @return Normalized vector.
     */
    template<typename VecType>
    inline VecType Normalize(const VecType& V) { return V.Normalized(); }

    /**
     * @brief Returns the dot product of two vectors.
     * @param A First vector.
     * @param B Second vector.
     * @return Scalar dot product.
     */
    template<typename VecType>
    inline float Dot(const VecType& A, const VecType& B) { return A.Dot(B); }

    /**
     * @brief Returns the cross product of two 3D vectors.
     * Only valid for 3D vectors.
     * @param A First vector.
     * @param B Second vector.
     * @return Vector perpendicular to both inputs.
     */
    inline FVector3 Cross(const FVector3& A, const FVector3& B) { return A.Cross(B); }

    /**
     * @brief Linearly interpolates between two vectors.
     * Uses the vector's Lerp method if available; otherwise uses basic formula.
     * @param A Start vector.
     * @param B End vector.
     * @param Alpha Interpolation factor [0,1].
     * @return Interpolated vector.
     */
    template<typename VecType>
    inline VecType Lerp(const VecType& A, const VecType& B, float Alpha) { return A.Lerp(B, Alpha); }

    /**
    * @brief Returns the distance between two vectors.
    * @param A First vector.
    * @param B Second vector.
    * @return Euclidean distance.
    */
    template<typename VecType>
    inline float Distance(const VecType& A, const VecType& B) { return (B - A).Length(); }

    /**
     * @brief Returns the squared distance between two vectors (avoids sqrt for efficiency).
     * @param A First vector.
     * @param B Second vector.
     * @return Squared distance.
     */
    template<typename VecType>
    inline float DistanceSquared(const VecType& A, const VecType& B) { return (B - A).LengthSquared(); }

    /**
     * @brief Returns the angle in radians between two normalized 3D vectors.
     * @param A First vector.
     * @param B Second vector.
     * @return Angle in radians.
     */
    inline float AngleBetween(const FVector3& A, const FVector3& B)
    {
        return glm::acos(FMath::Clamp(FMath::Dot(A.Normalized(), B.Normalized()), -1.0f, 1.0f));
    }

    /////////////////////
    // Matrix Utilities
    /////////////////////

    /**
     * @brief Returns the identity matrix.
     */
    inline FMatrix Identity() { return FMatrix::Identity(); }

    /**
     * @brief Returns the inverse of a matrix.
     */
    inline FMatrix Inverse(const FMatrix &M) { return M.Inverse(); }

    /**
     * @brief Returns the transpose of a matrix.
     */
    inline FMatrix Transpose(const FMatrix &M) { return M.Transpose(); }

    /**
     * @brief Returns a translation matrix.
     */
    inline FMatrix Translate(const FVector3 &T) { return FMatrix::Translate(T); }

    /**
     * @brief Returns a scaling matrix.
     */
    inline FMatrix Scale(const FVector3 &S) { return FMatrix::Scale(S); }

    /**
     * @brief Returns a rotation matrix from a quaternion.
     */
    inline FMatrix Rotate(const glm::quat& Q) { return FMatrix::Rotate(Q); }

    /**
     * @brief Returns an orthographic projection matrix.
     */
    inline FMatrix Ortho(float Left, float Right, float Bottom, float Top, float Near, float Far)
    {
        return FMatrix(glm::ortho(Left, Right, Bottom, Top, Near, Far));
    }

    /**
     * @brief Returns a perspective projection matrix.
     */
    inline FMatrix Perspective(float FOV, float Aspect, float Near, float Far)
    {
        return FMatrix(glm::perspective(glm::radians(FOV), Aspect, Near, Far));
    }

    /**
    * @brief Creates a look-at matrix.
    * @param Eye Position of the camera.
    * @param Target Target point to look at.
    * @param Up Up direction vector.
    * @return Look-at matrix.
    */
    inline FMatrix LookAt(const FVector3& Eye, const FVector3& Target, const FVector3& Up)
    {
        return FMatrix(glm::lookAt(glm::vec3(Eye.x, Eye.y, Eye.z),
                                   glm::vec3(Target.x, Target.y, Target.z),
                                   glm::vec3(Up.x, Up.y, Up.z)));
    }

    /////////////////////
    // Quaternion Utilities
    /////////////////////

    /**
     * @brief Spherical linear interpolation (SLERP) between two quaternions.
     * @param A Start quaternion.
     * @param B End quaternion.
     * @param Alpha Interpolation factor [0,1].
     * @return Interpolated quaternion.
     */
    inline FQuat Slerp(const FQuat& A, const FQuat& B, float Alpha)
    {
        return FQuat(glm::slerp(static_cast<glm::quat>(A), static_cast<glm::quat>(B), Alpha));
    }

    /////////////////////
    // Transform Utilities
    /////////////////////

    /**
     * @brief Converts an FTransform to an FMatrix.
     */
    inline FMatrix ToMatrix(const FTransform& T) { return T.ToMatrix(); }

    /**
     * @brief Converts an FMatrix to an FTransform.
     */
    inline FTransform FromMatrix(const FMatrix& M) { return FTransform::FromMatrix(M); }

    /**
    * @brief Combines two transforms (applies B after A).
    * @param A First transform.
    * @param B Second transform.
    * @return Combined transform.
    */
    inline FTransform Combine(const FTransform& A, const FTransform& B)
    {
        FMatrix Result = A.ToMatrix() * B.ToMatrix();
        return FTransform::FromMatrix(FMatrix(static_cast<glm::mat4>(Result)));
    }

    /**
     * @brief Returns the inverse of a transform.
     * @param T Transform to invert.
     * @return Inverted transform.
     */
    inline FTransform Inverse(const FTransform& T)
    {
        FMatrix Inv = T.ToMatrix().Inverse();
        return FTransform::FromMatrix(FMatrix(static_cast<glm::mat4>(Inv)));
    }
}
