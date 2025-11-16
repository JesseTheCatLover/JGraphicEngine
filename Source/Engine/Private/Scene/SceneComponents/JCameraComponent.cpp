// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/SceneComponents/JCameraComponent.h"
#include <algorithm>

#include "Core/JEngine.h"

JCameraComponent::JCameraComponent()
{
    m_AspectRatio = JEngine::Get().GetState().GetAspectRatio();
}

const FMatrix4& JCameraComponent::GetViewMatrix() const
{
    if (m_ViewDirty)
        RecalculateViewMatrix();
    return m_ViewMatrix;
}

const FMatrix4& JCameraComponent::GetProjectionMatrix() const
{
    if (m_ProjDirty)
        RecalculateProjectionMatrix();
    return m_ProjectionMatrix;
}

void JCameraComponent::RecalculateViewMatrix() const
{
    // We assume FTransform::ToMatrix() returns an FMatrix4 representing world transform:
    // world = Translate * Rotate * Scale (camera's world transform).
    // View is inverse(world).
    const FMatrix4 worldMat = GetWorldTransform().ToMatrix();
    m_ViewMatrix = worldMat.Inverse(); // requires FMatrix4::Inverse()
    m_ViewDirty = false;
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

    m_ProjDirty = false;
}

FVector3 JCameraComponent::GetForwardVector() const
{
    // Camera-local forward is -Z. Transform local -Z into world space.
    const FMatrix4 world = GetWorldTransform().ToMatrix();
    FVector3 forward = world.TransformVector(FVector3(0.0f, 0.0f, -1.0f));
    return forward.Normalized();
}

FVector3 JCameraComponent::GetUpVector() const
{
    // Camera-local up is +Y.
    const FMatrix4 world = GetWorldTransform().ToMatrix();
    FVector3 up = world.TransformVector(FVector3(0.0f, 1.0f, 0.0f));
    return up.Normalized();
}

FVector3 JCameraComponent::GetRightVector() const
{
    // Camera-local right is +X.
    const FMatrix4 world = GetWorldTransform().ToMatrix();
    FVector3 right = world.TransformVector(FVector3(1.0f, 0.0f, 0.0f));
    return right.Normalized();
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

    m_ViewDirty = true;
}

void JCameraComponent::SerializeProperties(JsonWriter& writer) const
{
    JSceneComponent::SerializeProperties(writer);

    writer.Write("projection_type", (int)m_ProjectionType);
    writer.Write("fov_degrees", m_FOV);
    writer.Write("aspect", m_AspectRatio);
    writer.Write("near_plane", m_NearClip);
    writer.Write("far_plane", m_FarClip);
    writer.Write("ortho_half_height", m_OrthoHalfHeight);
}

void JCameraComponent::DeserializeProperties(const JsonReader& reader)
{
    JSceneComponent::DeserializeProperties(reader);

    int projType = reader.Read("projection_type", (int)EProjectionType::Perspective);
    m_ProjectionType = static_cast<EProjectionType>(projType);
    m_FOV = reader.Read("fov_degrees", m_FOV);
    m_AspectRatio = reader.Read("aspect", m_AspectRatio);
    m_NearClip = reader.Read("near_plane", m_NearClip);
    m_FarClip = reader.Read("far_plane", m_FarClip);
    m_OrthoHalfHeight = reader.Read("ortho_half_height", m_OrthoHalfHeight);

    m_ViewDirty = true;
    m_ProjDirty = true;
}
