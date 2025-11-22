// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include "Scene/SceneComponents/JSceneComponent.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Core/Math/FMatrix4.h"
#include "Core/Math/FVector3.h"

#include "Rendering/ICameraViewSource.h"

/**
 * @brief Camera component for runtime/gameplay rendering in the scene
 */
class JCameraComponent : public JSceneComponent, public ICameraViewSource
{
    DECLARE_JOBJECT(JCameraComponent, JSceneComponent)

public:
    enum class EProjectionType { Perspective, Orthographic };

protected:
    EProjectionType m_ProjectionType = EProjectionType::Perspective;

    // Perspective
    float m_FOV = 60.0f;
    float m_AspectRatio = 16.0f / 9.0f;
    float m_NearClip = 0.01f;
    float m_FarClip = 1000.0f;

    // Orthographic
    float m_OrthoHalfHeight = 10.0f;

    // Cached matrices
    mutable FMatrix4 m_ViewMatrix = FMatrix4::Identity();
    mutable FMatrix4 m_ProjectionMatrix = FMatrix4::Identity();
    mutable bool m_bViewDirty = true;
    mutable bool m_bProjDirty = true;

public:
    JCameraComponent();
    virtual ~JCameraComponent() = default;

protected:
    void Initialize() override;

    // Matrices
    [[nodiscard]] const FMatrix4& GetViewMatrix() const override;
    [[nodiscard]] const FMatrix4& GetProjectionMatrix() const override;

    float GetNearPlane() const override { return m_NearClip; }
    float GetFarPlane() const override { return m_FarClip; }
    float GetOrthoHalfHeight() const override { return m_OrthoHalfHeight; }

public:
    // Getters
    EProjectionType GetProjectionType() const { return m_ProjectionType; }
    float GetFOV() const { return m_FOV; }
    float GetAspectRatio() const { return m_AspectRatio; }

    // Settters
    void SetProjectionType(EProjectionType type) { m_ProjectionType = type; m_bProjDirty = true; }
    void SetPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane)
    {
        m_FOV = fovDegrees;
        m_AspectRatio = aspect;
        m_NearClip = nearPlane;
        m_FarClip = farPlane;
        m_bProjDirty = true;
    }
    void SetPerspectiveFOV(float fovDegrees) { m_FOV = fovDegrees; m_bProjDirty = true; }
    void SetAspect(float aspect) { m_AspectRatio = aspect; m_bProjDirty = true; }
    void SetNearFar(float nearPlane, float farPlane) { m_NearClip = nearPlane; m_FarClip = farPlane; m_bProjDirty = true; }

    void SetOrthographic(float halfHeight, float aspect, float nearPlane, float farPlane)
    {
        m_OrthoHalfHeight = halfHeight;
        m_AspectRatio = aspect;
        m_NearClip = nearPlane;
        m_FarClip = farPlane;
        m_bProjDirty = true;
    }
    void SetOrthoHalfHeight(float halfHeight) { m_OrthoHalfHeight = halfHeight; m_bProjDirty = true; }

    // Recalculate both explicitly
    void RecalculateViewMatrix() const;
    void RecalculateProjectionMatrix() const;
    void RecalculateMatrices() const { RecalculateViewMatrix(); RecalculateProjectionMatrix(); }

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
    }

    // Serialization
    void SerializeCustom(JsonWriter& writer) const override;
    void Deserialize(const JsonReader& reader) override;
};
