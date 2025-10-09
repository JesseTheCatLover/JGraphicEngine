//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "FVector2.h"
#include "FVector3.h"
#include "FVector4.h"
#include "FMatrix.h"
#include "FRotator.h"
#include "FQuat.h"
#include "FEuler.h"
#include "FTransform.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

/**
 * @namespace FMath
 * @brief A math utility namespace for the engine. Provides vector, matrix, transform,
 * scalar, and angle functions fully wrapped for FMath utility types.
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

    /**
    * @brief Converts each component of a vector from degrees to radians.
    * @tparam VecType Type of the vector (must have x, y, z members).
    * @param DegreesVec Vector with angles in degrees.
    * @return Vector with angles converted to radians.
    */
    template<typename VecType>
    inline VecType Radians(const VecType& DegreesVec)
    {
        VecType out;
        out.x = glm::radians(DegreesVec.x);
        out.y = glm::radians(DegreesVec.y);
        out.z = glm::radians(DegreesVec.z);
        return out;
    }

    /**
     * @brief Converts each component of a vector from radians to degrees.
     * @tparam VecType Type of the vector (must have x, y, z members).
     * @param RadiansVec Vector with angles in radians.
     * @return Vector with angles converted to degrees.
     */
    template<typename VecType>
    inline VecType Degrees(const VecType& RadiansVec)
    {
        VecType out;
        out.x = glm::degrees(RadiansVec.x);
        out.y = glm::degrees(RadiansVec.y);
        out.z = glm::degrees(RadiansVec.z);
        return out;
    }

    /**
     * @brief Converts an FEuler rotation to radians..
    * @param Euler Input rotation in FEuler.
    * @return FEuler with all components in radians.
    */
    inline FEuler Radians(const FEuler& Euler)
    {
        return {FMath::Radians(Euler.Pitch),
                      FMath::Radians(Euler.Yaw),
                      FMath::Radians(Euler.Roll)};
    }

    /**
     * @brief Converts an FEuler rotation to degrees.
     * @param Euler Input rotation in FEuler.
     * @return FEuler with all components in degrees.
     */
    inline FEuler Degrees(const FEuler& Euler)
    {
        return {FMath::Degrees(Euler.Pitch),
                      FMath::Degrees(Euler.Yaw),
                      FMath::Degrees(Euler.Roll)};
    }

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
    // Euler Utilities
    /////////////////////

    /**
    * @brief Linearly interpolates between two FEuler rotations.
     * @param A Start rotation.
    * @param B End rotation.
    * @param Alpha Interpolation factor [0,1].
    * @return Interpolated FEuler rotation.
    */
    inline FEuler Lerp(const FEuler& A, const FEuler& B, float Alpha)
    {
        return FEuler(FMath::Lerp(A.Pitch, B.Pitch, Alpha),
                      FMath::Lerp(A.Yaw,   B.Yaw,   Alpha),
                      FMath::Lerp(A.Roll,  B.Roll,  Alpha));
    }


    /////////////////////
    // Transform Utilities
    /////////////////////

    /**
    * @brief Combines two transforms (applies B after A).
    * @param A First transform.
    * @param B Second transform.
    * @return Combined transform.
    */
    inline FTransform Combine(const FTransform& A, const FTransform& B)
    {
        FMatrix Result = A.ToMatrix() * B.ToMatrix();
        return FTransform::MakeFromMatrix(FMatrix(static_cast<glm::mat4>(Result)));
    }

    /**
     * @brief Returns the inverse of a transform.
     * @param T Transform to invert.
     * @return Inverted transform.
     */
    inline FTransform Inverse(const FTransform& T)
    {
        FMatrix Inv = T.ToMatrix().Inverse();
        return FTransform::MakeFromMatrix(FMatrix(static_cast<glm::mat4>(Inv)));
    }
}
