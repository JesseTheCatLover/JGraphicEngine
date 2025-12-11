//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

enum class EProjectionType { Perspective, Orthographic };

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
    [[nodiscard]] virtual float GetNearPlane() const = 0;
    [[nodiscard]] virtual float GetFarPlane() const = 0;
    [[nodiscard]] virtual float GetOrthoHalfHeight() const = 0;
    virtual void RebuildProjectionMatrix(float aspectRatio) const = 0;
};
