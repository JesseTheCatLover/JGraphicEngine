// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

enum ECameraMovement
{
    ECM_Forward,
    ECM_Backward,
    ECM_Right,
    ECM_Left,
    ECM_Up,
    ECM_Down
};

class JCamera
{
public:
    JCamera();
    JCamera(class Settings* Setting);
    JCamera(vec3 Position = vec3(0.f, 0.f, 0.f), vec3 Up = vec3(0.f , 1.f, 0.f),
            float Yaw = -90.f, float Pitch = 0.f);
    JCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    bool bFPS = false;
    float Yaw;
    float Pitch;
    float Speed;
    float xSensitivity;
    float ySensitivity;
    float Zoom;
    vec3 Position;
    vec3 Front;
    vec3 Up;
    vec3 Right;
    vec3 WorldUp;

    mat4 GetViewMatrix();
    void ProcessKeyboard(ECameraMovement Direction, float DeltaTime);
    void ProcessMouseMovement(float xOffset, float yOffset, bool ConstrainPitch = true);
    void ProcessMouseScroll(float yOffset, float MaxFOV);

private:
    void UpdateCameraVectors();
};
