//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Tools/CameraEditorTool.h"

#include <iostream>

#include "Core/JEngine.h"
#include "Core/Math/FMath.h"
#include "Framework/InputManager.h"

CameraEditorTool::CameraEditorTool()
{
    m_WorldTransform.SetPosition(0.f, 0.f, 15.f);

    m_Pitch = 0.f;
    m_Yaw = 0.f;

    m_FOV = 60.f;
    m_Near = 0.1f;
    m_Far = 10000.f;
    m_OrthoHalfHeight = 10.f;
    m_MoveSpeed = 20.f;
    m_ProjectionType = EProjectionType::Perspective;

    m_bViewDirty = true;
    m_bProjDirty = true;
}

void CameraEditorTool::Tick(float deltaTime, bool bActive, float viewportAspect)
{
    // Always keep proj up to date with aspect
    if (!FMath::IsNearlyEqual(viewportAspect, m_LastAspect))
    {
        m_LastAspect = viewportAspect;
        m_bProjDirty = true;      // lazily rebuilt on next GetProjectionMatrix() call
    }

    if (!bActive)
        return;

    InputManager* input = JEngine::Get().GetInputManager();
    if (!input) return;

    // Mouse look:
    FVector2 lookDelta = input->GetAxis2D("Editor_Look");
    if (lookDelta.x != 0.f || lookDelta.y != 0.f)
    {
        // Accumulate yaw/pitch in radians
        m_Yaw += lookDelta.x;
        m_Pitch -= lookDelta.y;

        // Clamp pitch in degrees, then convert back
        float pitchDeg = FMath::Degrees(m_Pitch);
        pitchDeg = FMath::Clamp(pitchDeg, -90.f, 90.f);
        m_Pitch = FMath::Radians(pitchDeg);

        // Rebuild orientation from Euler angles (roll = 0)
        FEuler euler(-m_Pitch, m_Yaw, 0.f);
        FQuat newRot = euler.ToQuat();

        m_WorldTransform.SetRotation(newRot);
        m_bViewDirty = true;
    }

    const FQuat rot = m_WorldTransform.GetRotation();

    const FVector3 forward = rot.RotateVector(FVector3::Forward()).Normalized();
    const FVector3 right = rot.RotateVector(FVector3::Right()).Normalized();
    const FVector3 up = FVector3::Up();

    FVector3 move(0.f, 0.f, 0.f);

    // WASD movement
    FVector2 moveInput = input->GetAxis2D("Editor_Move");   // X = W/S, Y = A/D
    float moveZ = input->GetAxis1D("Editor_MoveUpDown"); // Space / Shift

    move += forward * moveInput.x;      // W/S -> forward/back
    move += right   * moveInput.y;      // A/D -> strafe right/left
    move += up * moveZ;     // Space/Shift -> up/down

    if (move.Length() > 0.f)
    {
        move = move.Normalized() * m_MoveSpeed * deltaTime;
        m_WorldTransform.SetPosition(m_WorldTransform.GetPosition() + move);
        m_bViewDirty = true;
    }
}

FMatrix4& CameraEditorTool::GetViewMatrix() const
{
    if (m_bViewDirty)
        RebuildView();
    return m_View;
}

FMatrix4& CameraEditorTool::GetProjectionMatrix(float aspect) const
{
    if (m_bProjDirty || !FMath::IsNearlyEqual(aspect, m_LastAspect))
    {
        RebuildProj(aspect);
        m_LastAspect = aspect;
    }
    return m_Proj;
}

FProjectionDesc CameraEditorTool::GetProjectionDesc(float aspect) const
{
    FProjectionDesc d;
    d.type = (m_ProjectionType == EProjectionType::Perspective) ? EProjectionType::Perspective : EProjectionType::Orthographic;
    d.fovYDeg = m_FOV;
    d.orthoHalfHeight = m_OrthoHalfHeight;
    d.aspect = aspect;
    d.nearP = m_Near;
    d.farP = m_Far;
    return d;
}

void CameraEditorTool::RebuildView() const
{
    const FMatrix4 worldMat = m_WorldTransform.ToMatrix();
    m_View = worldMat.Inverse();
    m_bViewDirty = false;
}

void CameraEditorTool::RebuildProj(float aspect) const
{
    const float nearP = std::max(1e-6f, m_Near);
    const float farP  = std::max(nearP + 1e-6f, m_Far);

    if (m_ProjectionType == EProjectionType::Perspective)
    {
        m_Proj = FMath::Perspective(m_FOV, aspect, nearP, farP);
    }
    else // Orthographic
    {
        // Vertical size stays fixed; width scales with aspect
        float halfHeight = m_OrthoHalfHeight;
        float halfWidth = halfHeight * aspect;

        float left = -halfWidth;
        float right = +halfWidth;
        float bottom = -halfHeight;
        float top = +halfHeight;

        m_Proj = FMath::Ortho(left, right, bottom, top, m_Near, m_Far);
    }

    m_bProjDirty = false;
}