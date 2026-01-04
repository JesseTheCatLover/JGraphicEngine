//  Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <cmath>

#include "FMath.h"
#include "Core/Math/FVector3.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FMatrix4.h"
#include "Scene/FRaycast.h"
#include "Rendering/ICameraViewSource.h"

namespace FViewportMath
{
    inline FRay BuildRayFromCamera(const ICameraViewSource& cam,
                                   float viewportW, float viewportH,
                                   float xPx, float yPx)
    {
        FRay out{};
        if (viewportW <= 0.f || viewportH <= 0.f)
            return out;

        // pixel -> NDC (-1..1), y flipped because top-left origin
        const float xNDC =  2.0f * (xPx / viewportW) - 1.0f;
        const float yNDC =  1.0f - 2.0f * (yPx / viewportH);
        const float aspect = viewportW / viewportH;

        const FVector3 camPos = cam.GetPosition();
        const FQuat    camRot = cam.GetRotation();

        if (cam.GetProjectionType() == EProjectionType::Perspective)
        {
            const float halfFovRad = FMath::Radians(cam.GetFOV() * 0.5f);
            const float tanHalfFov = std::tan(halfFovRad);

            // Camera-space (ENGINE convention): +X forward, +Y right, +Z up
            FVector3 dirCam;
            dirCam.x = 1.0f;
            dirCam.y = xNDC * tanHalfFov * aspect;
            dirCam.z = yNDC * tanHalfFov;

            out.origin    = camPos;
            out.direction = camRot.RotateVector(dirCam).Normalized();
            return out;
        }
        else
        {
            // Orthographic: origin slides on view plane, direction is constant forward
            const float halfH = cam.GetOrthoHalfHeight();
            const float halfW = halfH * aspect;

            // point on camera plane (x = 0 in camera space)
            FVector3 pCam;
            pCam.x = 0.0f;
            pCam.y = xNDC * halfW;
            pCam.z = yNDC * halfH;

            out.origin    = camRot.RotateVector(pCam) + camPos;
            out.direction = camRot.RotateVector(FVector3(1,0,0)).Normalized(); // +X forward
            return out;
        }
    }
}