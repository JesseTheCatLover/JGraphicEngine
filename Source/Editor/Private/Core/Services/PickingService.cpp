//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "PickingService.h"

#include "SceneQueryService.h"
#include "Core/EditorHost.h"
#include "Core/Math/FMath.h"

PickingService::PickingService(EditorHost &host)
: m_Host(host)
{}

ActorID PickingService::PickActorAtViewportPos(const CameraEditorTool &cam, float width, float height, float x, float y) const
{
    if (width <= 0.f || height <= 0.f) return 0;

    FRay ray = BuildRay(cam, width, height, x, y);

    FRaycastHit hit{};
    auto& queries = m_Host.GetService<SceneQueryService>();
    if (queries.Raycast(ray, hit) && hit.bHit)
        return hit.actorID;

    return 0;
}

FRay PickingService::BuildRay(const CameraEditorTool& cam,
                              float width, float height,
                              float x, float y)
{
    FRay out{};

    if (width <= 0.f || height <= 0.f)
        return out;

    // 1) pixel -> NDC
    // x,y are [0..w],[0..h] with (0,0) at top-left
    const float xNDC =  2.0f * (x / width)  - 1.0f;
    const float yNDC =  1.0f - 2.0f * (y / height);

    const float aspect = width / height;

    // 2) camera basis
    const FQuat    camRot = cam.GetRotation();
    const FVector3 camPos = cam.GetPosition();

    FVector3 originWorld{};
    FVector3 dirWorld{};

    if (cam.GetProjectionType() == EProjectionType::Perspective)
    {
        const float verticalFovDeg = cam.GetFOV();
        const float halfFovRad = FMath::Radians(verticalFovDeg * 0.5f);
        const float tanHalfFov = std::tan(halfFovRad);

        // Camera space: X forward, Y right, Z up (your engine convention)
        FVector3 dirCam;
        dirCam.x = 1.0f;
        dirCam.y = xNDC * tanHalfFov * aspect;
        dirCam.z = yNDC * tanHalfFov;

        dirCam = dirCam.Normalized();

        dirWorld = camRot.RotateVector(dirCam).Normalized();
        originWorld = camPos;
    }
    else
    {
        // Orthographic:
        // origin slides on view plane, direction is constant forward
        const float halfHeight = cam.GetOrthoHalfHeight();
        const float halfWidth  = halfHeight * aspect;

        FVector3 pointCam;
        pointCam.x = 0.0f;
        pointCam.y = xNDC * halfWidth;
        pointCam.z = yNDC * halfHeight;

        const FVector3 pointWorld = camRot.RotateVector(pointCam) + camPos;

        const FVector3 forwardCam(1.0f, 0.0f, 0.0f);
        dirWorld = camRot.RotateVector(forwardCam).Normalized();

        originWorld = pointWorld;
    }

    out.origin = originWorld;
    out.direction = dirWorld;
    return out;
}