// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include "Scene/SceneComponents/JSceneComponent.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Core/Math/FMatrix4.h"
#include "Core/Math/FVector3.h"

/**
 * @brief Camera component providing view/projection matrices.
 */
class JCameraComponent : public JSceneComponent
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
    float m_FarClip  = 1000.0f;

    // Orthographic
    float m_OrthoHalfHeight = 10.0f;

    // Cached matrices
    mutable FMatrix4 m_ViewMatrix = FMatrix4::Identity();
    mutable FMatrix4 m_ProjectionMatrix = FMatrix4::Identity();
    mutable bool m_ViewDirty = true;
    mutable bool m_ProjDirty = true;

public:
    JCameraComponent();
    virtual ~JCameraComponent() = default;

    // Getters
    EProjectionType GetProjectionType() const { return m_ProjectionType; }
    float GetFOV() const { return m_FOV; }
    float GetAspectRatio() const { return m_AspectRatio; }
    float GetNearPlane() const { return m_NearClip; }
    float GetFarPlane() const { return m_FarClip; }
    float GetOrthoHalfHeight() const { return m_OrthoHalfHeight; }

    // Settters
    void SetProjectionType(EProjectionType type) { m_ProjectionType = type; m_ProjDirty = true; }
    void SetPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane)
    {
        m_FOV = fovDegrees;
        m_AspectRatio = aspect;
        m_NearClip = nearPlane;
        m_FarClip = farPlane;
        m_ProjDirty = true;
    }
    void SetPerspectiveFOV(float fovDegrees) { m_FOV = fovDegrees; m_ProjDirty = true; }
    void SetAspect(float aspect) { m_AspectRatio = aspect; m_ProjDirty = true; }
    void SetNearFar(float nearPlane, float farPlane) { m_NearClip = nearPlane; m_FarClip = farPlane; m_ProjDirty = true; }

    void SetOrthographic(float halfHeight, float aspect, float nearPlane, float farPlane)
    {
        m_OrthoHalfHeight = halfHeight;
        m_AspectRatio = aspect;
        m_NearClip = nearPlane;
        m_FarClip = farPlane;
        m_ProjDirty = true;
    }
    void SetOrthoHalfHeight(float halfHeight) { m_OrthoHalfHeight = halfHeight; m_ProjDirty = true; }

    // Matrices
    [[nodiscard]] const FMatrix4& GetViewMatrix() const;
    [[nodiscard]] const FMatrix4& GetProjectionMatrix() const;

    // Recalculate both explicitly
    void RecalculateViewMatrix() const;
    void RecalculateProjectionMatrix() const;
    void RecalculateMatrices() const { RecalculateViewMatrix(); RecalculateProjectionMatrix(); }

    // Utility
    FVector3 GetForwardVector() const; // camera forward in world space (engine convention: -Z forward assumed)
    FVector3 GetUpVector() const;
    FVector3 GetRightVector() const;

    // LookAt helper (sets world transform to look at target)
    void LookAt(const FVector3& worldTarget, const FVector3& worldUp = FVector3::Up());

protected:
    // When local transform changes, view becomes dirty.
    void OnLocalTransformChanged() override
    {
        Super::OnLocalTransformChanged();
        m_ViewDirty = true;
    }

    // Called when attached — ensure matrices marked dirty so they recalc on first use
    void OnAttachment() override
    {
        Super::OnAttachment();
        m_ViewDirty = true;
        m_ProjDirty = true;
    }

    // Serialization
    void SerializeProperties(JsonWriter& writer) const override;
    void DeserializeProperties(const JsonReader& reader) override;
};
