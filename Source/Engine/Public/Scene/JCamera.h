// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#pragma once

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

class JCamera // TODO: Deprecated, Will be replaced soon with a component style object
{
public:
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

    float FOV = 90.0f;
    float AspectRatio = 16.f/10.f;
    float NearClip = 0.1f;
    float FarClip = 1000.f;

    mat4 GetViewMatrix();
    mat4 GetProjectionMatrix() const;
    void ProcessKeyboard(ECameraMovement Direction, float DeltaTime);
    void ProcessMouseMovement(float xOffset, float yOffset, bool ConstrainPitch = true);
    void ProcessMouseScroll(float yOffset, float MaxFOV);

private:
    void UpdateCameraVectors();
};
