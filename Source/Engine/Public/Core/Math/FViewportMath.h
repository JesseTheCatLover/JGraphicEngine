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
    // Generic helper: build ray from matrices.
    inline FRay BuildRayFromViewProj(const FMatrix4& viewMat,
                                     const FMatrix4& projMat,
                                     float viewportW, float viewportH,
                                     float xPx, float yPx)
    {
        FRay out{};

        if (viewportW <= 0.f || viewportH <= 0.f)
            return out;

        // pixel -> NDC (-1..1), y flipped because top-left origin
        const float xNDC =  2.0f * (xPx / viewportW) - 1.0f;
        const float yNDC =  1.0f - 2.0f * (yPx / viewportH);

        const FMatrix4 VP = projMat * viewMat;
        const FMatrix4 invVP = VP.Inverse();

        const FVector4 pNearClip(xNDC, yNDC, -1.0f, 1.0f);
        const FVector4 pFarClip (xNDC, yNDC,  1.0f, 1.0f);

        FVector4 pNearW = invVP * pNearClip;
        FVector4 pFarW  = invVP * pFarClip;

        if (std::fabs(pNearW.w) <= 1e-6f || std::fabs(pFarW.w) <= 1e-6f)
            return out;

        pNearW = pNearW / pNearW.w;
        pFarW  = pFarW  / pFarW.w;

        const FVector3 nearWS(pNearW.x, pNearW.y, pNearW.z);
        const FVector3 farWS (pFarW.x,  pFarW.y,  pFarW.z);

        out.origin    = nearWS;
        out.direction = FMath::NormalizeSafe(farWS - nearWS);
        return out;
    }

    inline FRay BuildRayFromCamera(const ICameraViewSource& cam,
                                   float viewportW, float viewportH,
                                   float xPx, float yPx)
    {
        const float aspect = (viewportH > 0.f) ? (viewportW / viewportH) : 1.0f;

        //cam.RebuildProjectionMatrix(aspect);

        const FMatrix4& view = cam.GetViewMatrix();
        const FMatrix4& proj = cam.GetProjectionMatrix(aspect);

        return BuildRayFromViewProj(view, proj, viewportW, viewportH, xPx, yPx);
    }
}
