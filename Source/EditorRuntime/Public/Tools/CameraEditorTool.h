//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Core/Math/FMatrix4.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FVector3.h"
#include "Rendering/ICameraViewSource.h"
#include "Rendering/FViewportRT.h"

class CameraEditorTool : public ICameraViewSource
{
private:
    FTransform m_WorldTransform;
    float m_Yaw;
    float m_Pitch;

    float m_FOV;
    float m_Near;
    float m_Far;
    float m_OrthoHalfHeight;
    float m_MoveSpeed;

    EProjectionType m_ProjectionType;

    // Cached matrices
    mutable FMatrix4 m_View;
    mutable FMatrix4 m_Proj;

    mutable bool m_bViewDirty = true;
    mutable bool m_bProjDirty = true;
    mutable float m_LastAspect = -1.0f;

    void RebuildView() const;
    void RebuildProj(float aspect) const;

public:
    CameraEditorTool();

    // Called by editor core each frame
    void Tick(float deltaTime, bool bActive, float viewportAspect);

    // Control API
    void SetPosition(const FVector3& pos) { m_WorldTransform.SetPosition(pos); m_bViewDirty = true; }
    FVector3 GetPosition() const override { return m_WorldTransform.GetPosition(); }

    // void SetRotation(const FQuat& rot) { m_WorldTransform.SetRotation(rot); }
    // void SetRotation(const FRotator& rot) { SetRotation(rot.ToQuat()); }
    FQuat GetRotation() const override { return m_WorldTransform.GetRotation(); }

    void SetProjectionType(EProjectionType type) { m_ProjectionType = type; m_bProjDirty = true; }
    EProjectionType GetProjectionType() const { return m_ProjectionType; }

    void SetOrthoHalfHeight(float halfHeight) { m_OrthoHalfHeight = halfHeight; m_bProjDirty = true; }

    void SetSpeed(float speed) { m_MoveSpeed = speed; }
    float GetSpeed() const { return m_MoveSpeed; }

    void SetFOV(float degrees) { m_FOV = degrees; m_bProjDirty = true; }
    float GetFOV() const override { return m_FOV; }

    // ICameraViewSource
    FMatrix4& GetViewMatrix() const override;
    FMatrix4& GetProjectionMatrix(float aspectRatio) const override;
    [[nodiscard]] FProjectionDesc GetProjectionDesc(float aspect) const override;
    float GetNearPlane() const override { return m_Near; }
    float GetFarPlane() const override { return m_Far; }
    [[nodiscard]] float GetOrthoHalfHeight() const override { return m_OrthoHalfHeight; }
    void RebuildProjectionMatrix(float aspectRatio) const override { RebuildProj(aspectRatio); }
};
