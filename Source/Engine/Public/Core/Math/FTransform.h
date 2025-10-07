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
    // Accessors
    FVector3& position() { return m_Position; }
    FQuat& rotation() { return m_Rotation; }
    FVector3& scale() { return m_Scale; }

    [[nodiscard]] FVector3 position() const { return m_Position; }
    [[nodiscard]] FQuat rotation() const { return m_Rotation; }
    [[nodiscard]] FVector3 scale() const { return m_Scale; }

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
        glm::mat4 R = glm::toMat4(m_Rotation.operator glm::quat());;
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(m_Scale.x, m_Scale.y, m_Scale.z));
        return FMatrix(T * R * S);
    }

    /**
     * @brief Decomposes a matrix into a transform
     * @param matrix The matrix to decompose
     * @return FTransform containing position, rotation, and scale
     */
    static FTransform FromMatrix(const FMatrix& matrix)
    {
        glm::vec3 glmScale, glmTranslation, skew;
        glm::quat glmRotation;
        glm::vec4 perspective;

        const glm::mat4& glmMat = static_cast<const glm::mat4>(matrix); // internal GLM
        glm::decompose(glmMat, glmScale, glmRotation, glmTranslation, skew, perspective);

        return {FVector3(glmTranslation.x, glmTranslation.y, glmTranslation.z),
                          FQuat(glmRotation), FVector3(glmScale.x, glmScale.y, glmScale.z)};
    }

    // Operators
    bool operator==(const FTransform& Other) const
    {
        return m_Position == Other.m_Position && m_Rotation == Other.m_Rotation && m_Scale == Other.m_Scale;
    }

    bool operator!=(const FTransform& Other) const { return !(*this == Other); }

    /** Returns a string representation for debugging */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << "Position: " << m_Position.ToString() << ", "
           << "Rotation: " << m_Rotation.ToString() << ", "
           << "Scale: " << m_Scale.ToString();
        return ss.str();
    }
};
