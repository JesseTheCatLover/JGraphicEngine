//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "FProjectionDesc.h"
#include "Core/Math/FVector3.h"
#include "Core/Math/FQuat.h"
#include "Rendering/EProjectionType.h"

class RendererSubsystem;
struct FMatrix4;

/**
 * @brief Interface providing view/projection matrices and required parameters for rendering a view.
 */
class ICameraViewSource
{
public:
    virtual ~ICameraViewSource() = default;

    [[nodiscard]] virtual const FMatrix4& GetViewMatrix() const = 0;
    [[nodiscard]] virtual const FMatrix4& GetProjectionMatrix(float aspectRatio) const = 0;
    [[nodiscard]] virtual FProjectionDesc GetProjectionDesc(float aspect) const = 0;
    [[nodiscard]] virtual EProjectionType GetProjectionType() const = 0;
    [[nodiscard]] virtual FVector3 GetPosition() const = 0;
    [[nodiscard]] virtual FQuat GetRotation() const = 0;
    [[nodiscard]] virtual float GetFOV() const = 0;
    [[nodiscard]] virtual float GetNearPlane() const = 0;
    [[nodiscard]] virtual float GetFarPlane() const = 0;
    [[nodiscard]] virtual float GetOrthoHalfHeight() const = 0;
    virtual void RebuildProjectionMatrix(float aspectRatio) const = 0;
};
