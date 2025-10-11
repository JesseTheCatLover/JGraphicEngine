//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Math/FTransform.h"

#include "Core/Math/FEuler.h"
#include "Core/Math/FMatrix4.h"

void FTransform::SetRotation(const FEuler &euler)
{
    m_Rotation = euler.ToQuat();
}

void FTransform::SetRotation(const FVector3 &eulerVec)
{
    m_Rotation = FEuler::MakeFromVector3(eulerVec).ToQuat();
}

FMatrix4 FTransform::ToMatrix() const
{
    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(m_Position.x, m_Position.y, m_Position.z));
    glm::mat4 R = glm::toMat4(m_Rotation.operator glm::quat());
    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(m_Scale.x, m_Scale.y, m_Scale.z));
    return FMatrix4(T * R * S);
}

FTransform FTransform::MakeFromMatrix(const FMatrix4 &matrix)
{
    glm::vec3 glmScale, glmTranslation, skew;
    glm::quat glmRotation;
    glm::vec4 perspective;

    const glm::mat4& glmMat = static_cast<const glm::mat4>(matrix); // internal GLM
    glm::decompose(glmMat, glmScale, glmRotation, glmTranslation, skew, perspective);

    return {
        FVector3(glmTranslation.x, glmTranslation.y, glmTranslation.z),
        FQuat(glmRotation),
        FVector3(glmScale.x, glmScale.y, glmScale.z)
    };
}

FTransform FTransform::Inverse() const
{
    FMatrix4 invMatrix = ToMatrix().Inverse();
    return FTransform::MakeFromMatrix(invMatrix);
}

FVector3 FTransform::TransformPosition(const FVector3 &point) const
{
    return ToMatrix().TransformPoint(point);
}

FVector3 FTransform::TransformVector(const FVector3 &vector) const
{
    return ToMatrix().TransformVector(vector);
}
