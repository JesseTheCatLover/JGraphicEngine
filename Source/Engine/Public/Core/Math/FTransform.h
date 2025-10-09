// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "FVector3.h"
#include "FMatrix.h"
#include "FQuat.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <sstream>

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
    FQuat m_Rotation{1.f, 0.f, 0.f, 0.f};
    FVector3 m_Scale{1.f};

public:
    /// Accessors

    [[nodiscard]] const FVector3 GetPosition() const { return m_Position; }
    [[nodiscard]] const FQuat GetRotation() const { return m_Rotation; }
    [[nodiscard]] const FRotator GetRotationAsRotator() const { return GetRotation().ToRotator();}
    [[nodiscard]] const FVector3 GetScale() const { return m_Scale; }

    void SetPosition(const FVector3& position) { m_Position = position; }
    void SetRotation(const FQuat& rotation) { m_Rotation = rotation; }
    void SetRotation(const FEuler& euler) { m_Rotation = euler.ToQuat(); }
    void SetRotation(const FRotator& rotator) { SetRotation(FQuat::MakeFromRotator(rotator)); }
    void SetRotation(const FVector3& eulerVec) { m_Rotation = FEuler::MakeFromVector3(eulerVec).ToQuat(); }
    void SetScale(const FVector3& scale) { m_Scale = scale; }

    // Constructors
    FTransform() = default;

    FTransform(const FVector3& InPosition, const FQuat& InRotation, const FVector3& InScale)
        : m_Position(InPosition), m_Rotation(InRotation), m_Scale(InScale) {}

    /**
     * @brief Converts this transform into a 4x4 matrix (TRS order)
     * @return FMatrix representing the transform
     */
    [[nodiscard]] FMatrix ToMatrix() const
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(m_Position.x, m_Position.y, m_Position.z));
        glm::mat4 R = glm::toMat4(m_Rotation.operator glm::quat());
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(m_Scale.x, m_Scale.y, m_Scale.z));
        return FMatrix(T * R * S);
    }

    /**
     * @brief Decomposes a matrix into a transform
     * @param matrix The matrix to decompose
     * @return FTransform containing position, rotation, and scale
     */
    static FTransform MakeFromMatrix(const FMatrix& matrix)
    {
        glm::vec3 glmScale, glmTranslation, skew;
        glm::quat glmRotation;
        glm::vec4 perspective;

        const glm::mat4& glmMat = static_cast<const glm::mat4>(matrix); // internal GLM
        glm::decompose(glmMat, glmScale, glmRotation, glmTranslation, skew, perspective);

        return {FVector3(glmTranslation.x, glmTranslation.y, glmTranslation.z),
                FQuat(glmRotation),
                FVector3(glmScale.x, glmScale.y, glmScale.z)};
    }

    /**
    * @brief Returns the inverse of this transform.
    * @return Inverted transform.
    */
    [[nodiscard]] FTransform Inverse() const
    {
        FMatrix invMatrix = ToMatrix().Inverse();
        return FTransform::MakeFromMatrix(invMatrix);
    }

    [[nodiscard]] FVector3 TransformPosition(const FVector3& point) const
    {
        return ToMatrix().TransformPoint(point);
    }

    [[nodiscard]] FVector3 TransformVector(const FVector3& vector) const
    {
        return ToMatrix().TransformVector(vector);
    }

    /**
     * @brief Combines two transforms by applying the second transform (B) after the first (A).
     *
     * This operator computes the equivalent transform as if you first applied transform A,
     * and then applied transform B in the local space of A. The combination respects
     * position, rotation, and scale in 3D space:
     *
     * - The position of B is scaled by A's scale, rotated by A's rotation, and then
     *   offset by A's position.
     * - The rotations are combined using quaternion multiplication (A followed by B).
     * - The scales are combined component-wise.
     *
     * Mathematically:
     *   combined_position = A.position + A.rotation.RotateVector(B.position * A.scale)
     *   combined_rotation = A.rotation * B.rotation
     *   combined_scale    = A.scale * B.scale
     *
     * @param A The first transform to apply.
     * @param B The second transform to apply after A.
     * @return A new FTransform representing the combination of A and B.
     *
     * @note This does NOT modify either A or B. Use this to compute world transforms
     *       from local transforms or to chain transformations hierarchically.
     */
    inline FTransform operator*(const FTransform& a, const FTransform& b)
    {
        FVector3 rotatedPos = a.GetRotation().RotateVector(b.GetPosition() * a.GetScale());
        FVector3 combinedPos = a.GetPosition() + rotatedPos;

        FQuat combinedRot = a.GetRotation() * b.GetRotation();
        FVector3 combinedScale = a.GetScale() * b.GetScale();

        return {combinedPos, combinedRot, combinedScale};
    }

    bool operator==(const FTransform& other) const
    {
        return m_Position == other.m_Position && m_Rotation == other.m_Rotation && m_Scale == other.m_Scale;
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
