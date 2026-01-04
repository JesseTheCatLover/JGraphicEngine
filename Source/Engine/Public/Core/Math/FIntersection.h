//  Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cmath>
#include <algorithm>

#include "Core/Math/FVector3.h"
#include "Core/Math/FMatrix4.h"
#include "Scene/FRaycast.h"

namespace FIntersections
{
    constexpr float kEps = 1e-6f;

    // ------------------------------------------------------------
    // Ray vs Plane
    // Plane: point P0 and normal N (N should be normalized)
    // Returns t along ray (origin + direction*t), t >= 0.
    // ------------------------------------------------------------
    inline bool RayPlane(const FRay& ray,
                         const FVector3& planePoint,
                         const FVector3& planeNormal,
                         float& outT,
                         FVector3* outHitPoint = nullptr,
                         float eps = kEps)
    {
        const float denom = planeNormal.Dot(ray.direction);
        if (std::fabs(denom) < eps)
            return false; // parallel

        const float t = (planePoint - ray.origin).Dot(planeNormal) / denom;
        if (t < 0.0f)
            return false; // behind

        outT = t;
        if (outHitPoint)
            *outHitPoint = ray.origin + ray.direction * t;
        return true;
    }

    // ------------------------------------------------------------
    // Ray vs Sphere (useful for center handles / free rotate)
    // Returns nearest positive t.
    // ------------------------------------------------------------
    inline bool RaySphere(const FRay& ray,
                          const FVector3& center,
                          float radius,
                          float& outT,
                          float eps = kEps)
    {
        const FVector3 oc = ray.origin - center;
        const float a = ray.direction.Dot(ray.direction); // should be 1 if normalized
        const float b = 2.0f * oc.Dot(ray.direction);
        const float c = oc.Dot(oc) - radius * radius;

        const float disc = b*b - 4.0f*a*c;
        if (disc < 0.0f) return false;

        const float sqrtDisc = std::sqrt(disc);

        // smallest positive root
        float t0 = (-b - sqrtDisc) / (2.0f*a);
        float t1 = (-b + sqrtDisc) / (2.0f*a);

        if (t0 > t1) std::swap(t0, t1);

        if (t0 >= 0.0f) { outT = t0; return true; }
        if (t1 >= 0.0f) { outT = t1; return true; }

        return false;
    }

    // ------------------------------------------------------------
    // Ray vs AABB (slab method)
    // AABB defined by min/max in same space as the ray
    // Returns nearest positive t.
    // ------------------------------------------------------------
    inline bool RayAABB(const FRay& ray,
                        const FVector3& bmin,
                        const FVector3& bmax,
                        float& outT,
                        float eps = kEps)
    {
        float tmin = 0.0f;
        float tmax = 1e30f;

        auto slab = [&](float ro, float rd, float mn, float mx) -> bool
        {
            if (std::fabs(rd) < eps)
            {
                // Ray parallel to slab: must be inside
                return (ro >= mn && ro <= mx);
            }

            float inv = 1.0f / rd;
            float t0 = (mn - ro) * inv;
            float t1 = (mx - ro) * inv;
            if (t0 > t1) std::swap(t0, t1);

            tmin = std::max(tmin, t0);
            tmax = std::min(tmax, t1);
            return (tmin <= tmax);
        };

        if (!slab(ray.origin.x, ray.direction.x, bmin.x, bmax.x)) return false;
        if (!slab(ray.origin.y, ray.direction.y, bmin.y, bmax.y)) return false;
        if (!slab(ray.origin.z, ray.direction.z, bmin.z, bmax.z)) return false;

        // choose first hit >= 0
        outT = (tmin >= 0.0f) ? tmin : tmax;
        return outT >= 0.0f;
    }

    // ------------------------------------------------------------
    // Ray vs OBB by transforming ray into OBB local space then AABB test
    // OBB is defined by center + basis axes (unit) + halfExtents.
    // This avoids requiring a full matrix
    // ------------------------------------------------------------
    inline bool RayOBB(const FRay& ray,
                       const FVector3& center,
                       const FVector3& axisX, // unit
                       const FVector3& axisY, // unit
                       const FVector3& axisZ, // unit
                       const FVector3& halfExtents,
                       float& outT,
                       float eps = kEps)
    {
        // Convert ray into OBB local coordinates where box is AABB [-he..+he]
        const FVector3 p = ray.origin - center;

        FRay local;
        local.origin = FVector3(
            p.Dot(axisX),
            p.Dot(axisY),
            p.Dot(axisZ)
        );

        local.direction = FVector3(
            ray.direction.Dot(axisX),
            ray.direction.Dot(axisY),
            ray.direction.Dot(axisZ)
        );

        const FVector3 bmin = -halfExtents;
        const FVector3 bmax =  halfExtents;

        return RayAABB(local, bmin, bmax, outT, eps);
    }

    // ------------------------------------------------------------
    // Closest point on a ray to a point (for measuring “distance to axis”)
    // ------------------------------------------------------------
    inline FVector3 ClosestPointOnRay(const FRay& ray, const FVector3& point, float& outT)
    {
        outT = (point - ray.origin).Dot(ray.direction.Normalized());
        if (outT < 0.0f) outT = 0.0f;
        return ray.origin + ray.direction * outT;
    }
}
