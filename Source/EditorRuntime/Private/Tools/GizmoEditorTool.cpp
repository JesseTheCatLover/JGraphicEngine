//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Tools/GizmoEditorTool.h"

#include <algorithm>
#include <cmath>

// ----------------------
// Utility
// ----------------------

float GizmoEditorTool::ComputeGizmoScale(const FVector3& camPos, const FVector3& gizmoPos)
{
    const float dist = (gizmoPos - camPos).Length();
    return std::max(0.15f, dist * 0.10f);
}

FVector4 GizmoEditorTool::LerpRGB(const FVector4& a, const FVector4& b, float t)
{
    const float it = 1.0f - t;
    return FVector4(
        a.x * it + b.x * t,
        a.y * it + b.y * t,
        a.z * it + b.z * t,
        a.w
    );
}

static bool IsInActiveGroup(GizmoEditorTool::EHandle active, GizmoEditorTool::EHandle h)
{
    using H = GizmoEditorTool::EHandle;
    if (active == H::None || h == H::None) return false;
    if (h == active) return true;

    // Translate planes -> include both axes
    if (active == H::T_XY) return (h == H::T_X || h == H::T_Y);
    if (active == H::T_XZ) return (h == H::T_X || h == H::T_Z);
    if (active == H::T_YZ) return (h == H::T_Y || h == H::T_Z);

    // Scale planes -> include both axes
    if (active == H::S_XY) return (h == H::S_X || h == H::S_Y);
    if (active == H::S_XZ) return (h == H::S_X || h == H::S_Z);
    if (active == H::S_YZ) return (h == H::S_Y || h == H::S_Z);

    // Rotate: usually only the ring itself is active

    // Center/uniform: treat only itself as active group
    return false;
}
FVector4 GizmoEditorTool::ApplyHandleTint(EHandle h,
                                         const FDrawParams& p,
                                         const FVisualConfig& v,
                                         const FVector4& base) const
{
    FVector4 c = base;
    c.w *= p.alphaMul;

    const bool hasActive = (p.activeHandle != EHandle::None);
    const bool inActiveGroup = IsInActiveGroup(p.activeHandle, h);
    const bool isActiveHandle = (h != EHandle::None && h == p.activeHandle);
    const bool isHover = (h != EHandle::None && h == p.hoveredHandle);

    // While dragging: dim everything NOT in the active group
    if (p.bDimOthersWhenActive && hasActive && !inActiveGroup)
    {
        c = LerpRGB(c, v.inactiveGray, std::clamp(v.inactiveToGrayWhenActive, 0.0f, 1.0f));
        c.w = std::min(1.0f, c.w * v.inactiveAlphaMulWhenActive);
        return c;
    }

    // Active handle itself becomes yellow
    if (isActiveHandle)
    {
        c = LerpRGB(c, v.activeColor, std::clamp(v.activeBlend, 0.0f, 1.0f));
        c.w = std::min(1.0f, c.w * v.activeAlphaMul * v.activeColor.w);
        return c;
    }

    // If it’s in the active group (axis mates), make it “active-ish” too.
    // Two options:
    // A) Make mates fully yellow like the plane (UE-ish)
    // B) Make mates partially yellow (so you can still tell which handle is actually grabbed)
    if (p.bDimOthersWhenActive && hasActive && inActiveGroup)
    {
        constexpr float kMateBlend = 0.75f; // tweak; 1.0 == identical to active
        c = LerpRGB(c, v.activeColor, kMateBlend);
        c.w = std::min(1.0f, c.w * v.activeAlphaMul);
        // still allow hover (rare during drag)
        return c;
    }

    // Hover (only meaningful when not dragging)
    if (isHover)
    {
        const FVector4 white(1,1,1,1);
        c = LerpRGB(c, white, std::clamp(v.hoverToWhite, 0.0f, 1.0f));
        c.w = std::min(1.0f, c.w * v.hoverAlphaMul);
        return c;
    }

    return c;
}

FDebugDrawStyle GizmoEditorTool::ApplyHandleStyle(EHandle h, const FDrawParams& p, const FVisualConfig& v, FDebugDrawStyle s) const
{
    if (h != EHandle::None && h == p.hoveredHandle)
        s.thicknessPx += v.hoverThickAddPx;
    return s;
}

void GizmoEditorTool::BuildBasis(const FTransform& xf, ESpace space, FVector3& outX, FVector3& outY, FVector3& outZ)
{
    const FQuat q = (space == ESpace::Local) ? xf.GetRotation() : FQuat{};

    const FVector3 AX = FVector3::Forward();
    const FVector3 AY = FVector3::Right();
    const FVector3 AZ = FVector3::Up();

    outX = q.RotateVector(AX).Normalized();
    outY = q.RotateVector(AY).Normalized();
    outZ = q.RotateVector(AZ).Normalized();
}

void GizmoEditorTool::DrawPlaneSquareWire(DebugDraw& dd,
                                          const FVector3& origin,
                                          const FVector3& axisA,
                                          const FVector3& axisB,
                                          float halfSize,
                                          const FVector4& color,
                                          const FDebugDrawStyle& style)
{
    const FVector3 a = axisA * halfSize;
    const FVector3 b = axisB * halfSize;

    const FVector3 p0 = origin - a - b;
    const FVector3 p1 = origin + a - b;
    const FVector3 p2 = origin + a + b;
    const FVector3 p3 = origin - a + b;

    dd.DrawLine(p0, p1, color, style);
    dd.DrawLine(p1, p2, color, style);
    dd.DrawLine(p2, p3, color, style);
    dd.DrawLine(p3, p0, color, style);
}

void GizmoEditorTool::DrawPlaneSquareSolid(DebugDraw& dd,
                                           const FVector3& origin,
                                           const FVector3& axisA,
                                           const FVector3& axisB,
                                           float halfSize,
                                           const FVector4& color,
                                           FDebugDrawStyle style)
{
    const FVector3 a = axisA * halfSize;
    const FVector3 b = axisB * halfSize;

    const FVector3 p0 = origin - a - b;
    const FVector3 p1 = origin + a - b;
    const FVector3 p2 = origin + a + b;
    const FVector3 p3 = origin - a + b;

    style.fill = EDebugFillMode::Solid;
    style.shading = EDebugShading::Unlit;
    dd.DrawQuad(p0, p1, p2, p3, color, style);
}

static void DrawCircleArc(DebugDraw& dd,
                          const FVector3& center,
                          const FVector3& planeNormalUnit,
                          const FVector3& inPlaneZeroDirUnit, // direction at angle=0 in the plane
                          float radius,
                          float startAngleRad,
                          float endAngleRad,
                          int segments,
                          const FVector4& color,
                          const FDebugDrawStyle& style)
{
    const FVector3 N = planeNormalUnit.Normalized();

    // Build in-plane orthonormal basis (U,V) where U is angle=0 direction
    FVector3 U = inPlaneZeroDirUnit - N * inPlaneZeroDirUnit.Dot(N);
    if (U.Dot(U) < 1e-6f)
    {
        // pick any stable in-plane axis if caller gave degenerate vector
        FVector3 any = (std::fabs(N.z) < 0.9f) ? FVector3(0,0,1) : FVector3(0,1,0);
        U = any - N * any.Dot(N);
    }
    U = U.Normalized();
    const FVector3 V = N.Cross(U).Normalized();

    segments = std::max(8, segments);

    FVector3 prev{};
    bool hasPrev = false;

    for (int i = 0; i <= segments; ++i)
    {
        const float a = float(i) / float(segments);
        const float t = startAngleRad + (endAngleRad - startAngleRad) * a;

        const float ct = std::cos(t);
        const float st = std::sin(t);

        const FVector3 pt = center + (U * ct + V * st) * radius;

        if (hasPrev)
            dd.DrawLine(prev, pt, color, style);

        prev = pt;
        hasPrev = true;
    }
}

static void DrawCircleFrontHalf(DebugDraw& dd,
                                const FVector3& center,
                                const FVector3& ringPlaneNormalUnit,
                                float radius,
                                const FVector3& camPos,
                                int segments,
                                const FVector4& color,
                                const FDebugDrawStyle& style,
                                float arcHalfAngleRad = 0.5f) // default = half circle
{
    arcHalfAngleRad *= 3.14159265358979323846f;
    const FVector3 N = ringPlaneNormalUnit.Normalized();

    FVector3 toCam = camPos - center;
    FVector3 p = toCam - N * toCam.Dot(N); // projection into plane

    if (p.Dot(p) < 1e-6f)
    {
        FVector3 any = (std::fabs(N.z) < 0.9f) ? FVector3(0,0,1) : FVector3(0,1,0);
        p = any - N * any.Dot(N);
        if (p.Dot(p) < 1e-6f)
            p = FVector3(1,0,0) - N * FVector3(1,0,0).Dot(N);
    }

    const FVector3 U = p.Normalized(); // angle=0 faces camera in plane

    // Draw an arc centered around U, with total angle = 2*arcHalfAngleRad
    const float start = -arcHalfAngleRad;
    const float end   = +arcHalfAngleRad;

    DrawCircleArc(dd, center, N, U, radius, start, end, segments, color, style);
}

// ----------------------
// HitId mapping
// ----------------------

uint32_t GizmoEditorTool::HandleToHitId(uint32_t baseHitId, EHandle h) const
{
    if (baseHitId == 0) return 0;

    switch (h)
    {
        // Translate (base + 1..7)
        case EHandle::T_X:      return baseHitId + 1;
        case EHandle::T_Y:      return baseHitId + 2;
        case EHandle::T_Z:      return baseHitId + 3;
        case EHandle::T_XY:     return baseHitId + 4;
        case EHandle::T_XZ:     return baseHitId + 5;
        case EHandle::T_YZ:     return baseHitId + 6;
        case EHandle::T_Center: return baseHitId + 7;

        // Rotate (base + 10..13)
        case EHandle::R_X:      return baseHitId + 10;
        case EHandle::R_Y:      return baseHitId + 11;
        case EHandle::R_Z:      return baseHitId + 12;
        case EHandle::R_Free:   return baseHitId + 13;

        // Scale (base + 20..26)
        case EHandle::S_X:       return baseHitId + 20;
        case EHandle::S_Y:       return baseHitId + 21;
        case EHandle::S_Z:       return baseHitId + 22;
        case EHandle::S_XY:      return baseHitId + 23;
        case EHandle::S_XZ:      return baseHitId + 24;
        case EHandle::S_YZ:      return baseHitId + 25;
        case EHandle::S_Uniform: return baseHitId + 26;

        default: return 0;
    }
}

GizmoEditorTool::EHandle GizmoEditorTool::HitIdToHandle(uint32_t baseHitId, uint32_t hitId) const
{
    if (baseHitId == 0 || hitId == 0) return EHandle::None;

    const uint32_t d = hitId - baseHitId;

    switch (d)
    {
        case 1:  return EHandle::T_X;
        case 2:  return EHandle::T_Y;
        case 3:  return EHandle::T_Z;
        case 4:  return EHandle::T_XY;
        case 5:  return EHandle::T_XZ;
        case 6:  return EHandle::T_YZ;
        case 7:  return EHandle::T_Center;

        case 10: return EHandle::R_X;
        case 11: return EHandle::R_Y;
        case 12: return EHandle::R_Z;
        case 13: return EHandle::R_Free;

        case 20: return EHandle::S_X;
        case 21: return EHandle::S_Y;
        case 22: return EHandle::S_Z;
        case 23: return EHandle::S_XY;
        case 24: return EHandle::S_XZ;
        case 25: return EHandle::S_YZ;
        case 26: return EHandle::S_Uniform;

        default: return EHandle::None;
    }
}

// ----------------------
// Draw entry
// ----------------------

void GizmoEditorTool::Draw(DebugDraw& dd, uint32_t viewKey, const FVector3& camPos, const FVector3& camFwd,
                           const FTransform& gizmoXf, const FDrawParams& p)
{
    FVisualConfig v{};
    const FVector3 gizmoPos = gizmoXf.GetPosition();
    float scale = ComputeGizmoScale(camPos, gizmoPos);
    scale *= p.scaleMul;

    switch (p.mode)
    {
        case EMode::Translate:
            DrawTranslate(dd, viewKey, camPos, camFwd, gizmoXf, p, scale, v);
            break;
        case EMode::Rotate:
            DrawRotate(dd, viewKey, camPos, camFwd, gizmoXf, p, scale, v);
            break;
        case EMode::Scale:
            DrawScale(dd, viewKey, camPos, camFwd, gizmoXf, p, scale, v);
            break;
        default: break;
    }
}

// ----------------------
// Translate
// ----------------------

void GizmoEditorTool::DrawTranslate(DebugDraw& dd,
                                    uint32_t viewKey,
                                    const FVector3& camPos,
                                    const FVector3& camFwd,
                                    const FTransform& xf,
                                    const FDrawParams& p,
                                    float gizmoScale,
                                    const FVisualConfig& v) const
{
    (void)camPos; (void)camFwd;

    FVector3 X, Y, Z;
    BuildBasis(xf, p.space, X, Y, Z);

    const FVector3 o = xf.GetPosition();

    const float axisLen   = v.axisLen       * gizmoScale;
    const float headLen   = v.headLen       * gizmoScale;
    const float headRad   = v.headRadius    * gizmoScale;
    const float centerRad = v.centerRadius  * gizmoScale;

    FDebugDrawStyle s{};
    s.layer = EDebugDrawLayer::Editor;
    s.depth = EDebugDepthMode::Overlay;
    s.fill = EDebugFillMode::Solid;
    s.thicknessPx = v.axisThicknessPx;
    s.shading = v.shading;
    s.normalMode = v.normalMode;
    s.viewKey = viewKey;

    const FVector4 cx(0.85f, 0.10f, 0.10f, 1.0f);
    const FVector4 cy(0.10f, 0.85f, 0.10f, 1.0f);
    const FVector4 cz(0.10f, 0.10f, 0.85f, 1.0f);
    const FVector4 cCenter(0.85f, 0.85f, 0.85f, 1.0f);

    const auto DrawAxisArrow = [&](EHandle handle, const FVector3& axisUnit, const FVector4& baseCol)
    {
        FDebugDrawStyle ps = s;
        ps.hitId = HandleToHitId(p.baseHitID, handle);
        ps = ApplyHandleStyle(handle, p, v, ps);

        const FVector4 col = ApplyHandleTint(handle, p, v, baseCol);
        dd.DrawArrow(o, o + axisUnit * axisLen, col, headLen, headRad, 16, ps);
    };

    const auto DrawPlaneHandle = [&](EHandle handle,
                                     const FVector3& axisA,
                                     const FVector3& axisB,
                                     const FVector4& baseCol)
    {
        const float half = (v.planeSize * 0.5f) * gizmoScale;
        const float off  = v.planeOffset * gizmoScale;

        FDebugDrawStyle ps = s;
        ps.thicknessPx = 2.0f;
        ps.hitId = HandleToHitId(p.baseHitID, handle);
        ps = ApplyHandleStyle(handle, p, v, ps);

        const FVector4 col = ApplyHandleTint(handle, p, v, baseCol);
        const FVector3 c = o + (axisA + axisB) * off;

        DrawPlaneSquareWire(dd, c, axisA, axisB, half, col, ps);
        DrawPlaneSquareSolid(dd, c, axisA, axisB, half, col, ps);
    };

    // Axis arrows
    DrawAxisArrow(EHandle::T_X, X, cx);
    DrawAxisArrow(EHandle::T_Y, Y, cy);
    DrawAxisArrow(EHandle::T_Z, Z, cz);

    // Center
    if (p.bDrawCenter)
    {
        FDebugDrawStyle cs = s;
        cs.thicknessPx = 1.0f;
        cs.hitId = HandleToHitId(p.baseHitID, EHandle::T_Center);
        cs = ApplyHandleStyle(EHandle::T_Center, p, v, cs);

        const FVector4 col = ApplyHandleTint(EHandle::T_Center, p, v, cCenter);
        dd.DrawSphere(o, centerRad, col, 24, cs);
    }

    // Plane handles
    if (p.bDrawPlanes)
    {
        const FVector4 cxy(0.0f, 0.0f, 1.0f, v.planeAlpha);
        const FVector4 cxz(0.0f, 1.0f, 0.0f, v.planeAlpha);
        const FVector4 cyz(1.0f, 0.0f, 0.0f, v.planeAlpha);

        DrawPlaneHandle(EHandle::T_XY, X, Y, cxy);
        DrawPlaneHandle(EHandle::T_XZ, X, Z, cxz);
        DrawPlaneHandle(EHandle::T_YZ, Y, Z, cyz);
    }
}

// ----------------------
// Rotate
// ----------------------

void GizmoEditorTool::DrawRotate(DebugDraw& dd,
                                 uint32_t viewKey,
                                 const FVector3& camPos,
                                 const FVector3& camFwd,
                                 const FTransform& xf,
                                 const FDrawParams& p,
                                 float gizmoScale,
                                 const FVisualConfig& v) const
{
    (void)camPos; (void)camFwd;

    FVector3 X, Y, Z;
    BuildBasis(xf, p.space, X, Y, Z);

    const FVector3 o = xf.GetPosition();

    const float weakScale = std::max(gizmoScale * 0.9f, 0.7f);
    const float r = v.ringRadius * weakScale;

    FDebugDrawStyle s{};
    s.layer = EDebugDrawLayer::Editor;
    s.depth = EDebugDepthMode::Overlay;
    s.fill  = EDebugFillMode::Wireframe;
    s.thicknessPx = v.ringThicknessPx;
    s.shading = v.shading;
    s.normalMode = v.normalMode;
    s.viewKey = viewKey;

    const FVector4 cx(0.85f, 0.10f, 0.10f, 1.0f);
    const FVector4 cy(0.10f, 0.85f, 0.10f, 1.0f);
    const FVector4 cz(0.10f, 0.10f, 0.85f, 1.0f);

    if (p.bDrawSphereHint)
    {
        FDebugDrawStyle hs = s;
        hs.thicknessPx = 1.0f;
        hs.hitId = 0;
        hs.fill = EDebugFillMode::Solid;

        const FVector4 hint(0.9f, 0.9f, 0.9f, v.sphereHintAlpha);
        dd.DrawSphere(o, r, hint, v.ringSegments, hs);
    }
    // X ring (plane normal = X)
    {
        FDebugDrawStyle rs = s;
        rs.hitId = HandleToHitId(p.baseHitID, EHandle::R_X);
        rs = ApplyHandleStyle(EHandle::R_X, p, v, rs);

        const FVector4 col = ApplyHandleTint(EHandle::R_X, p, v, cx);
        DrawCircleFrontHalf(dd, o, X, r, camPos, v.ringSegments, col, rs, v.ringArcHalfLength);
    }

    // Y ring
    {
        FDebugDrawStyle rs = s;
        rs.hitId = HandleToHitId(p.baseHitID, EHandle::R_Y);
        rs = ApplyHandleStyle(EHandle::R_Y, p, v, rs);

        const FVector4 col = ApplyHandleTint(EHandle::R_Y, p, v, cy);
        DrawCircleFrontHalf(dd, o, Y, r, camPos, v.ringSegments, col, rs, v.ringArcHalfLength);
    }

    // Z ring
    {
        FDebugDrawStyle rs = s;
        rs.hitId = HandleToHitId(p.baseHitID, EHandle::R_Z);
        rs = ApplyHandleStyle(EHandle::R_Z, p, v, rs);

        const FVector4 col = ApplyHandleTint(EHandle::R_Z, p, v, cz);
        DrawCircleFrontHalf(dd, o, Z, r, camPos, v.ringSegments, col, rs, v.ringArcHalfLength);
    }

    // Free rotate ring (camera-facing)
    {
        // Plane normal points toward camera
        FVector3 N = (camPos - o).Normalized();  // better than camFwd for orbit cameras
        if (N.Dot(N) < 1e-6f) N = camFwd.Normalized();

        FDebugDrawStyle rs = s;
        rs.thicknessPx *= 0.8;
        rs.hitId = HandleToHitId(p.baseHitID, EHandle::R_Free);
        rs = ApplyHandleStyle(EHandle::R_Free, p, v, rs);

        // White like center handles
        FVector4 cFree(0.85f, 0.85f, 0.85f, 0.75f);
        const FVector4 col = ApplyHandleTint(EHandle::R_Free, p, v, cFree);

        // pick a stable in-plane direction for angle=0:
        // project X basis into the plane so it doesn't spin randomly
        FVector3 ref = X; // or FVector3::Right() etc.
        ref = (ref - N * ref.Dot(N));
        if (ref.Dot(ref) < 1e-6f) ref = (Y - N * Y.Dot(N));
        ref = ref.Normalized();

        const float rFree = r * 1.1f; // slight outer rim feel

         DrawCircleFrontHalf(dd, o, N, rFree, camPos, v.ringSegments, col, rs,
             v.ringArcHalfLength * 0.5f);
    }


}

// ----------------------
// Scale
// ----------------------
void GizmoEditorTool::DrawScale(DebugDraw& dd,
                                uint32_t viewKey,
                                const FVector3& camPos,
                                const FVector3& camFwd,
                                const FTransform& xf,
                                const FDrawParams& p,
                                float gizmoScale,
                                const FVisualConfig& v) const
{
    (void)camPos; (void)camFwd;

    FVector3 X, Y, Z;
    BuildBasis(xf, p.space, X, Y, Z);

    const FVector3 o = xf.GetPosition();
    const float axisLen = v.scaleArmLen * gizmoScale;

    // Base styles
    FDebugDrawStyle sLine{};
    sLine.layer = EDebugDrawLayer::Editor;
    sLine.depth = EDebugDepthMode::Overlay;
    sLine.thicknessPx = v.scaleArmThicknessPx;
    sLine.shading = v.shading;
    sLine.normalMode = v.normalMode;
    sLine.viewKey = viewKey;

    FDebugDrawStyle sBox = sLine;
    sBox.fill = EDebugFillMode::Solid;
    sBox.thicknessPx = 1.0f;

    const float headHalf = v.scaleBoxHalf * gizmoScale;

    // Colors
    const FVector4 cx(0.85f, 0.10f, 0.10f, 1.0f);
    const FVector4 cy(0.10f, 0.85f, 0.10f, 1.0f);
    const FVector4 cz(0.10f, 0.10f, 0.85f, 1.0f);
    const FVector4 cU(0.90f, 0.90f, 0.90f, 0.85f);

    // Helpers
    const auto DrawAxisScale = [&](EHandle handle, const FVector3& axisUnit, const FVector4& baseCol)
    {
        const FVector3 end = o + axisUnit * axisLen;

        // Arm
        {
            FDebugDrawStyle ls = sLine;
            ls.hitId = HandleToHitId(p.baseHitID, handle);
            ls = ApplyHandleStyle(handle, p, v, ls);

            const FVector4 col = ApplyHandleTint(handle, p, v, baseCol);
            dd.DrawLine(o, end, col, ls);
        }

        // Head
        {
            FDebugDrawStyle bs = sBox;
            bs.hitId = HandleToHitId(p.baseHitID, handle);
            bs = ApplyHandleStyle(handle, p, v, bs);

            const FVector4 col = ApplyHandleTint(handle, p, v, baseCol);
            dd.DrawBox(end, FVector3(headHalf, headHalf, headHalf), col, bs);
        }
    };

    const auto DrawBiAxisScaleLine = [&](EHandle handle,
                                         const FVector3& aUnit,
                                         const FVector3& bUnit,
                                         const FVector4& baseCol,
                                         float off)
    {
        FDebugDrawStyle ps = sBox; // using sBox because you wanted "thicker" line + solid config
        ps.thicknessPx = 2.0f;
        ps.hitId = HandleToHitId(p.baseHitID, handle);
        ps = ApplyHandleStyle(handle, p, v, ps);

        const FVector4 col = ApplyHandleTint(handle, p, v, baseCol);
        dd.DrawLine(o + aUnit * off, o + bUnit * off, col, ps);
    };

    // Axis handles
    DrawAxisScale(EHandle::S_X, X, cx);
    DrawAxisScale(EHandle::S_Y, Y, cy);
    DrawAxisScale(EHandle::S_Z, Z, cz);

    // Uniform center
    if (p.bDrawCenter)
    {
        FDebugDrawStyle bs = sBox;
        bs.hitId = HandleToHitId(p.baseHitID, EHandle::S_Uniform);
        bs = ApplyHandleStyle(EHandle::S_Uniform, p, v, bs);

        const FVector4 col = ApplyHandleTint(EHandle::S_Uniform, p, v, cU);
        const float h = v.uniformBoxHalf * gizmoScale;
        dd.DrawBox(o, FVector3(h, h, h), col, bs);
    }

    // Plane (bi-axis) handles
    if (p.bDrawPlanes)
    {
        const float off = v.scaleBiAxisLineScale * gizmoScale;

        // NOTE: these colors look swapped vs translate (fine if intentional)
        const FVector4 cxy(0.10f, 0.10f, 0.85f, v.planeAlpha);
        const FVector4 cxz(0.10f, 0.85f, 0.10f, v.planeAlpha);
        const FVector4 cyz(0.85f, 0.10f, 0.10f, v.planeAlpha);

        DrawBiAxisScaleLine(EHandle::S_XY, X, Y, cxy, off);
        DrawBiAxisScaleLine(EHandle::S_XZ, X, Z, cxz, off);
        DrawBiAxisScaleLine(EHandle::S_YZ, Y, Z, cyz, off);
    }
}