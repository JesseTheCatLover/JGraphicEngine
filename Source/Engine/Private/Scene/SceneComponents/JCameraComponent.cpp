// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/SceneComponents/JCameraComponent.h"
#include <algorithm>

#include "Core/JEngine.h"

JCameraComponent::JCameraComponent()
{
    m_AspectRatio = JEngine::Get().GetState().GetAspectRatio();
}

void JCameraComponent::Initialize()
{
    Super::Initialize();
}

const FMatrix4& JCameraComponent::GetViewMatrix() const
{
    if (m_bViewDirty)
        RecalculateViewMatrix();
    return m_ViewMatrix;
}

const FMatrix4& JCameraComponent::GetProjectionMatrix() const
{
    if (m_bProjDirty)
        RecalculateProjectionMatrix();
    return m_ProjectionMatrix;
}

void JCameraComponent::RecalculateViewMatrix() const
{
    const FMatrix4 worldMat = GetWorldTransform().ToMatrix();
    m_ViewMatrix = worldMat.Inverse();
    m_bViewDirty = false;
}

void JCameraComponent::RecalculateProjectionMatrix() const
{
    // clamp safe values
    const float nearP = std::max(1e-6f, m_NearClip);
    const float farP = std::max(nearP + 1e-6f, m_FarClip);

    if (m_ProjectionType == EProjectionType::Perspective)
    {
        m_ProjectionMatrix = FMath::Perspective(m_FOV, m_AspectRatio, nearP, farP);
    }
    else // Orthographic
    {
        const float halfH = m_OrthoHalfHeight;
        const float halfW = halfH * m_AspectRatio;

        const float left = -halfW;
        const float right = halfW;
        const float bottom = -halfH;
        const float top = halfH;

        m_ProjectionMatrix = FMath::Ortho(left, right, bottom, top, nearP, farP);
    }

    m_bProjDirty = false;
}

FVector3 JCameraComponent::GetForwardVector() const
{
    const FTransform worldT = GetWorldTransform();
    const FQuat rot = worldT.GetRotation();
    return rot.RotateVector(FVector3::Forward()).Normalized();
}

FVector3 JCameraComponent::GetRightVector() const
{
    const FTransform worldT = GetWorldTransform();
    const FQuat rot = worldT.GetRotation();
    return rot.RotateVector(FVector3::Right()).Normalized();
}

FVector3 JCameraComponent::GetUpVector() const
{
    const FTransform worldT = GetWorldTransform();
    const FQuat rot = worldT.GetRotation();
    return rot.RotateVector(FVector3::Up()).Normalized();
}

void JCameraComponent::LookAt(const FVector3& worldTarget, const FVector3& worldUp)
{
    const FVector3 eye = GetWorldPosition();

    if ((worldTarget - eye).Length() < 1e-12f)
        return;

    FMatrix4 view = FMath::LookAt(eye, worldTarget, worldUp);
    FMatrix4 worldFromView = view.Inverse();

    FTransform newWorld = FTransform::MakeFromMatrix(worldFromView);
    SetWorldTransform(newWorld);

    m_bViewDirty = true;
}

void JCameraComponent::SerializeCustom(JsonWriter& writer) const
{
    JSceneComponent::SerializeCustom(writer);

    writer.Write("projection_type", (int)m_ProjectionType);
    writer.Write("fov_degrees", m_FOV);
    writer.Write("aspect", m_AspectRatio);
    writer.Write("near_plane", m_NearClip);
    writer.Write("far_plane", m_FarClip);
    writer.Write("ortho_half_height", m_OrthoHalfHeight);
}

void JCameraComponent::Deserialize(const JsonReader& reader)
{
    JSceneComponent::Deserialize(reader);

    int projType = reader.Read("projection_type", (int)EProjectionType::Perspective);
    m_ProjectionType = static_cast<EProjectionType>(projType);
    m_FOV = reader.Read("fov_degrees", m_FOV);
    m_AspectRatio = reader.Read("aspect", m_AspectRatio);
    m_NearClip = reader.Read("near_plane", m_NearClip);
    m_FarClip = reader.Read("far_plane", m_FarClip);
    m_OrthoHalfHeight = reader.Read("ortho_half_height", m_OrthoHalfHeight);

    m_bViewDirty = true;
    m_bProjDirty = true;
}
