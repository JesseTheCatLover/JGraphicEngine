// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
#pragma once

#include "Scene/SceneComponents/JSceneComponent.h"
#include "Core/Math/FMatrix4.h"
#include "Core/Math/FVector3.h"

#include "Rendering/ICameraViewSource.h"
#include "JCameraComponent.generated.h"

/**
 * @brief Camera component for runtime/gameplay rendering in the scene
 */
JCLASS()
class JCameraComponent : public JSceneComponent, public ICameraViewSource
{
    GENERATED_BODY()

    JPROPERTY(Category("Projection"))
    EProjectionType m_ProjectionType;

    // Perspective
    JPROPERTY(Category("Projection"))
    float m_FOV;
    JPROPERTY(Category("Projection"))
    float m_NearClip;
    JPROPERTY(Category("Projection"))
    float m_FarClip;

    // Orthographic
    JPROPERTY(Category("Projection"))
    float m_OrthoHalfHeight;

    // Cached matrices
    mutable FMatrix4 m_ViewMatrix = FMatrix4::Identity();
    mutable FMatrix4 m_ProjectionMatrix = FMatrix4::Identity();
    mutable bool m_bViewDirty = true;
    mutable bool m_bProjDirty = true;
    mutable float m_LastAspect = -1.0f;

public:
    JCameraComponent();
    virtual ~JCameraComponent() = default;

protected:
    void Initialize() override;

    // Matrices
    [[nodiscard]] const FMatrix4& GetViewMatrix() const override;

    [[nodiscard]] const FMatrix4& GetProjectionMatrix(float aspectRatio) const override;

    [[nodiscard]] FProjectionDesc GetProjectionDesc(float aspect) const override;

protected:
    void OnWorldTransformChanged() override;

    float GetNearPlane() const override { return m_NearClip; }
    float GetFarPlane() const override { return m_FarClip; }
    float GetOrthoHalfHeight() const override { return m_OrthoHalfHeight; }

public:
    // Getters
    EProjectionType GetProjectionType() const override { return m_ProjectionType; }
    float GetFOV() const override { return m_FOV; }

    [[nodiscard]] FVector3 GetPosition() const override
    {
        return  m_WorldTransform.GetPosition();
    }

    [[nodiscard]] FQuat GetRotation() const override
    {
        return  m_WorldTransform.GetRotation();
    }

    // Setters
    void SetProjectionType(EProjectionType type) { m_ProjectionType = type; m_bProjDirty = true; }
    void SetPerspective(float fovDegrees, float nearPlane, float farPlane)
    {
        m_FOV = fovDegrees;
        m_NearClip = nearPlane;
        m_FarClip = farPlane;
        m_bProjDirty = true;
    }
    void SetPerspectiveFOV(float fovDegrees) { m_FOV = fovDegrees; m_bProjDirty = true; }
    void SetNearFar(float nearPlane, float farPlane) { m_NearClip = nearPlane; m_FarClip = farPlane; m_bProjDirty = true; }

    void SetOrthographic(float halfHeight, float nearPlane, float farPlane)
    {
        m_OrthoHalfHeight = halfHeight;
        m_NearClip = nearPlane;
        m_FarClip = farPlane;
        m_bProjDirty = true;
    }
    void SetOrthoHalfHeight(float halfHeight) { m_OrthoHalfHeight = halfHeight; m_bProjDirty = true; }

    // RebuildMatrices
    void RebuildViewMatrix() const;
    void RebuildProjectionMatrix(float aspectRatio) const override;
    void RebuildMatrices(float aspectRatio) const
    { RebuildViewMatrix(); RebuildProjectionMatrix(aspectRatio); }

    // Utility
    FVector3 GetForwardVector() const;
    FVector3 GetUpVector() const;
    FVector3 GetRightVector() const;

    // LookAt helper (sets world transform to look at target)
    void LookAt(const FVector3& worldTarget, const FVector3& worldUp = FVector3::Up());

protected:
    // When local transform changes, view becomes dirty.
    void OnLocalTransformChanged() override
    {
        Super::OnLocalTransformChanged();
        m_bViewDirty = true;
    }

    // Called when attached, ensure matrices marked dirty so they recalc on first use
    void OnAttachment() override
    {
        Super::OnAttachment();
        m_bViewDirty = true;
        m_bProjDirty = true;
        m_LastAspect  = -1.0f; // Force reprojection on next GetProjectionMatrix()
    }
};
