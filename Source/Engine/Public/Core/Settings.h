// Copyright (c) 2024 JesseTheCatLover. All Rights Reserved.

#pragma once

class Settings
{
public:
    struct S_Viewport
    {
        bool bWireframe = false;
    };

    struct S_Camera
    {
        const float Yaw = -90.f;
        const float Pitch = 0.f;
        const float Speed = 8.5f;
        const float xSensitivity = 0.1f;
        const float ySensitivity = 0.1f;
        const float MaxFOV = 45.f;
        const float Zoom = 45.f;
    };

    Settings();

private:
    // Setting properties
    S_Camera Camera;

public:
    inline const float GetCameraYaw() const { return Camera.Yaw; }
    inline const float GetCameraPitch() const { return Camera.Pitch; }
    inline const float GetCameraSpeed() const { return Camera.Speed; }
    inline const float GetCameraXSensitivity() const { return Camera.xSensitivity; }
    inline const float GetCameraYSensitivity() const { return Camera.ySensitivity; }
    inline const float GetCameraMaxFOV() const { return Camera.MaxFOV; }
    inline const float GetCameraZoom() const { return Camera.Zoom; }
};


