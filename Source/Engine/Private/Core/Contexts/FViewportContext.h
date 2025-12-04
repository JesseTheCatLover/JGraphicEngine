// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class ICameraViewSource;

struct FViewportContext
{
private:
    friend class EngineContext;

    int sceneViewportWidth = 0;
    int sceneViewportHeight = 0;
    bool bWireframe = false;
    float xSensitivity = 0.1f;
    float ySensitivity = 0.1f;
    float MaxFOV = 45.f;
    float Zoom = 45.f;
};
