//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/Contexts/EViewMode.h"
#include "Scene/JCamera.h"

struct FViewportContext
{
private:
    friend class EngineState;

    bool bWireframe = false;
    EViewMode viewMode = EViewMode::Scene;

    JCamera camera;
    float Yaw = -90.f;
    float Pitch = 0.f;
    float Speed = 8.5f;
    float xSensitivity = 0.1f;
    float ySensitivity = 0.1f;
    float MaxFOV = 45.f;
    float Zoom = 45.f;

public:
    // --- Yaw ---
    float GetYaw() const;
    void SetYaw(float yaw);

    // --- Pitch ---
    float GetPitch() const;
    void SetPitch(float pitch);

    // --- Speed ---
    float GetSpeed() const;
    void SetSpeed(float speed);

    // --- X Sensitivity ---
    float GetXSensitivity() const;
    void SetXSensitivity(float sens);

    // --- Y Sensitivity ---
    float GetYSensitivity() const;
    void SetYSensitivity(float sens);

    // --- Max FOV ---
    float GetMaxFOV() const;
    void SetMaxFOV(float fov);

    // --- Zoom ---
    float GetZoom() const;
    void SetZoom(float zoom);
};
