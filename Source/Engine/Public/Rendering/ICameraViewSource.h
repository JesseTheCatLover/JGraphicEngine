//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class RendererSubsystem;

/**
 * @brief Interface providing view/projection matrices and required parameters for rendering a view.
 */
class ICameraViewSource
{
    friend RendererSubsystem;

public:
    virtual ~ICameraViewSource() = default;

protected:
    [[nodiscard]] virtual const FMatrix4& GetViewMatrix(float aspectRatio) const = 0;
    [[nodiscard]] virtual const FMatrix4& GetProjectionMatrix(float aspectRatio) const = 0;
    [[nodiscard]] virtual float GetNearPlane() const = 0;
    [[nodiscard]] virtual float GetFarPlane() const = 0;
    [[nodiscard]] virtual float GetOrthoHalfHeight() const = 0;
};
