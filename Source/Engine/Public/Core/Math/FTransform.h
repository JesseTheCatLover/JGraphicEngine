// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "FVector3.h"
#include "FRotator.h"
#include "FQuat.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <sstream>

struct FMatrix4;

/**
 * @struct FTransform
 * @brief Represents a 3D transformation (position, rotation, scale).
 *
 * Wraps GLM internally for fast matrix/quaternion operations, but exposes
 * the engine API with FVector3, FQuat, and FMatrix.
 */
struct FTransform
{
private:
    FVector3 m_Position{0.f};
    FQuat m_Rotation{0.f, 0.f, 0.f, 1.f};  // Identity quaternion by default
    FVector3 m_Scale{1.f};

public:
    /// Accessors

    [[nodiscard]] FVector3 GetPosition() const { return m_Position; }
    [[nodiscard]] FQuat GetRotation() const { return m_Rotation; }
    [[nodiscard]] FRotator GetRotationAsRotator() const { return GetRotation().ToRotator();}
    [[nodiscard]] FVector3 GetScale() const { return m_Scale; }

    void SetPosition(const FVector3& position) { m_Position = position; }
    void SetPosition(const float &x, const float &y, const float &z) { m_Position = {x, y , z}; }
    void SetRotation(const FQuat& rotation) { m_Rotation = rotation; }
    void SetRotation(const FEuler& euler);
    void SetRotation(const FRotator& rotator) { SetRotation(FQuat::MakeFromRotator(rotator)); }
    void SetRotation(const FVector3& eulerVec);
    void SetScale(const FVector3& scale) { m_Scale = scale; }

    // Constructors
    FTransform() = default;

    FTransform(const FVector3& InPosition, const FQuat& InRotation, const FVector3& InScale)
        : m_Position(InPosition), m_Rotation(InRotation), m_Scale(InScale) {}

    /**
     * @brief Converts this transform into a 4x4 matrix (TRS order)
     * @return FMatrix representing the transform
     */
    [[nodiscard]] FMatrix4 ToMatrix() const;

    /**
     * @brief Decomposes a matrix into a transform
     * @param matrix The matrix to decompose
     * @return FTransform containing position, rotation, and scale
     */
    static FTransform MakeFromMatrix(const FMatrix4& matrix);

    /**
    * @brief Returns the inverse of this transform.
    * @return Inverted transform.
    */
    [[nodiscard]] FTransform Inverse() const;

    [[nodiscard]] FVector3 TransformPosition(const FVector3& point) const;

    [[nodiscard]] FVector3 TransformVector(const FVector3& vector) const;

    [[nodiscard]] FVector3 GetForward() const
    {
        return m_Rotation.RotateVector(FVector3(1, 0, 0));
    }

    [[nodiscard]] FVector3 GetRight() const
    {
        return m_Rotation.RotateVector(FVector3(0, 1, 0));
    }

    [[nodiscard]] FVector3 GetUp() const
    {
        return m_Rotation.RotateVector(FVector3(0, 0, 1));
    }

    inline FTransform operator*(const FTransform& other) const
    {
        FVector3 rotatedPos = m_Rotation.RotateVector(other.m_Position * m_Scale);
        FVector3 combinedPos = m_Position + rotatedPos;

        FQuat combinedRot = m_Rotation * other.m_Rotation;
        FVector3 combinedScale = m_Scale * other.m_Scale;

        return {combinedPos, combinedRot, combinedScale};
    }

    bool operator==(const FTransform& other) const
    {
        return m_Position == other.m_Position
        && m_Rotation == other.m_Rotation
        && m_Scale == other.m_Scale;
    }

    bool operator!=(const FTransform& other) const { return !(*this == other); }

    /** Returns a string representation */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << "Position: " << m_Position.ToString() << ", "
           << "Rotation: " << m_Rotation.ToString() << ", "
           << "Scale: " << m_Scale.ToString();
        return ss.str();
    }
};
