// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#include <iostream>
#include "JCamera.h"
#include "Settings.h"

JCamera::JCamera():
Yaw(-90.f),
Pitch(0.f),
Speed(8.5f),
xSensitivity(0.1f),
ySensitivity(0.1f),
Zoom(45.f),
Front(vec3(0.f, 0.f, -1.f)),
WorldUp(vec3(0.f, 1.f, 0.f))
{
    UpdateCameraVectors();
}

JCamera::JCamera(Settings* Setting)
{
    Yaw = Setting -> GetCameraYaw();
    Pitch = Setting -> GetCameraPitch();
    Speed = Setting -> GetCameraSpeed();
    xSensitivity = Setting -> GetCameraXSensitivity();
    ySensitivity = Setting -> GetCameraYSensitivity();
    Zoom = Setting -> GetCameraZoom();
    Front = vec3(0.f, 0.f, -1.f);
    WorldUp = vec3(0.0f, 1.0f, 0.0f);

    UpdateCameraVectors();
}

JCamera::JCamera(vec3 Position, vec3 Up, float Yaw, float Pitch):
Speed(8.5f),
xSensitivity(0.1f),
ySensitivity(0.1f),
Zoom(45.f),
Front(vec3(0.f, 0.f, -1.f))
{
    this->Position = Position; WorldUp = Up;
    this->Yaw = Yaw; this->Pitch = Pitch;
    UpdateCameraVectors();
}

JCamera::JCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch):
Speed(8.5f),
xSensitivity(0.1f),
ySensitivity(0.1f),
Zoom(45.f),
Front(vec3(0.f, 0.f, -1.f))
{
    Position = vec3(posX, posY, posZ);
    WorldUp = vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    UpdateCameraVectors();
}

mat4 JCamera::GetViewMatrix()
{
    return lookAt(Position, Position + Front, Up);
}

void JCamera::ProcessKeyboard(ECameraMovement Direction, float DeltaTime)
{
    float Velocity = Speed * DeltaTime;
    if(Direction == ECM_Forward)
        Position += Front * Velocity;
    if(Direction == ECM_Backward)
        Position -= Front * Velocity;
    if(Direction == ECM_Right)
        Position += Right * Velocity;
    if(Direction == ECM_Left)
        Position -= Right * Velocity;

    if(bFPS) Position.y = 0.f;
    else
    {
        if(Direction == ECM_Up)
            Position += WorldUp * Velocity;
        if(Direction == ECM_Down)
            Position -= WorldUp * Velocity;
    }
}

void JCamera::ProcessMouseMovement(float xOffset, float yOffset, bool ConstrainPitch)
{
    xOffset *= xSensitivity;
    yOffset *= ySensitivity;

    Yaw = mod(Yaw + xOffset, 360.0f);
    Pitch += yOffset;

    if(ConstrainPitch) // avoid screen flipping
    {
        if(Pitch > 89.f)
            Pitch = 89.f;
        if(Pitch < -89.f)
            Pitch = -89.f;
    }

    UpdateCameraVectors();
}

void JCamera::ProcessMouseScroll(float yOffset, float MaxFOV)
{
    Zoom -= (float)yOffset;
    if(Zoom < 1.f)
        Zoom = 1.f;
    if(Zoom > MaxFOV)
        Zoom = MaxFOV;
}

void JCamera::UpdateCameraVectors()
{
    vec3 front;
    front.x = cos(radians(Yaw)) * cos(radians(Pitch));
    front.y = sin(radians(Pitch));
    front.z = sin(radians(Yaw)) * cos(radians(Pitch));
    Front = normalize(front);
    // also re-calculate the Right and Up vector
    Right = normalize(cross(Front, WorldUp));
    Up    = normalize(cross(Right, Front));
}
