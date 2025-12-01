// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class ICameraViewSource;

struct FViewportContext
{
private:
    friend class EngineContext;

    bool bWireframe = false;

    ICameraViewSource* camera;
    int sceneViewportWidth = 0;
    int sceneViewportHeight = 0;
    float xSensitivity = 0.1f;
    float ySensitivity = 0.1f;
    float MaxFOV = 45.f;
    float Zoom = 45.f;
};
