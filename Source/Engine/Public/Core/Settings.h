// Copyright (c) 2024 JesseTheCatLover. All Rights Reserved.

#pragma once

class Settings
{
public:
    struct S_Window
    {
        unsigned int ScreenWidth;
        unsigned int ScreenHeight;
    };

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
    Settings(unsigned int Width, unsigned int Height, bool bWireframe);
    Settings(S_Window Win, S_Viewport View);
    Settings(S_Window Win, S_Viewport View, S_Camera Camera);

private:
    // Setting properties
    S_Window Window;
    S_Viewport Viewport;
    S_Camera Camera;

public:
    inline const unsigned int GetScreenWidth() const { return Window.ScreenWidth; }
    inline void SetScreenWidth(bool Value) { Window.ScreenWidth = Value; }
    inline const unsigned int GetScreenHeight() const { return Window.ScreenHeight; }
    inline void SetScreenHeight(bool Value) { Window.ScreenHeight = Value; }
    inline bool GetbWireFrame() const { return Viewport.bWireframe; }
    inline void SetbWireFrame(bool Value) { Viewport.bWireframe = Value; }
    inline const float GetCameraYaw() const { return Camera.Yaw; }
    inline const float GetCameraPitch() const { return Camera.Pitch; }
    inline const float GetCameraSpeed() const { return Camera.Speed; }
    inline const float GetCameraXSensitivity() const { return Camera.xSensitivity; }
    inline const float GetCameraYSensitivity() const { return Camera.ySensitivity; }
    inline const float GetCameraMaxFOV() const { return Camera.MaxFOV; }
    inline const float GetCameraZoom() const { return Camera.Zoom; }
};


