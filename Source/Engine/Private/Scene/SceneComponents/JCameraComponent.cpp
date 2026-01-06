// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/SceneComponents/JCameraComponent.h"
#include <algorithm>

#include "Core/Math/FMath.h"

JCameraComponent::JCameraComponent()
{
}

void JCameraComponent::Initialize()
{
    Super::Initialize();
}

const FMatrix4& JCameraComponent::GetViewMatrix() const
{
    if (m_bViewDirty)
        RebuildViewMatrix();
    return m_ViewMatrix;
}

const FMatrix4& JCameraComponent::GetProjectionMatrix(float aspectRatio) const
{
    // Rebuild if params changed OR aspect changed meaningfully
    if (m_bProjDirty || !FMath::IsNearlyEqual(aspectRatio, m_LastAspect))
    {
        RebuildProjectionMatrix(aspectRatio);
        m_LastAspect = aspectRatio;
    }
    return m_ProjectionMatrix;
}

FProjectionDesc JCameraComponent::GetProjectionDesc(float aspect) const
{
    FProjectionDesc d;
    d.type = (m_ProjectionType == EProjectionType::Perspective) ? EProjectionType::Perspective : EProjectionType::Orthographic;
    d.fovYDeg = m_FOV;
    d.orthoHalfHeight = m_OrthoHalfHeight;
    d.aspect = aspect;
    d.nearP = m_NearClip;
    d.farP = m_FarClip;
    return d;
}

void JCameraComponent::OnWorldTransformChanged()
{
    JSceneComponent::OnWorldTransformChanged();
    m_bViewDirty = true;
}

void JCameraComponent::RebuildViewMatrix() const
{
    const FMatrix4 worldMat = GetWorldTransform().ToMatrix();
    m_ViewMatrix = worldMat.Inverse();
    m_bViewDirty = false;
}

void JCameraComponent::RebuildProjectionMatrix(float aspectRatio) const
{
    // clamp safe values
    const float nearP = std::max(1e-6f, m_NearClip);
    const float farP = std::max(nearP + 1e-6f, m_FarClip);

    if (m_ProjectionType == EProjectionType::Perspective)
    {
        m_ProjectionMatrix = FMath::Perspective(m_FOV, aspectRatio, nearP, farP);
    }
    else // Orthographic
    {
        const float halfH = m_OrthoHalfHeight;
        const float halfW = halfH * aspectRatio;

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

JREFLECT_TYPE(JCameraComponent)
{
    JPROPERTY(m_FOV);
    JPROPERTY(m_NearClip);
    JPROPERTY(m_FarClip);
    JPROPERTY(m_ProjectionType); // TODO: implement enum reflection support + structs for future
    JPROPERTY(m_OrthoHalfHeight);
}}