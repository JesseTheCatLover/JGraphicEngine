//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Tools/GizmoEditorTool.h"

#include <algorithm>
#include <cmath>

float GizmoEditorTool::ComputeGizmoScale(const FVector3& camPos, const FVector3& gizmoPos)
{
    // Simple constant-size heuristic.
    // Tune these later; feels decent in practice.
    const float dist = (gizmoPos - camPos).Length();
    return std::max(0.15f, dist * 0.10f);
}

FVector4 GizmoEditorTool::LerpRGB(const FVector4 &a, const FVector4 &b, float t)
{
    const float it = 1.0f - t;
    return FVector4(
        a.x * it + b.x * t,
        a.y * it + b.y * t,
        a.z * it + b.z * t,
        a.w // keep alpha managed outside
    );
}

FVector4 GizmoEditorTool::ApplyHandleTint(EHandle h, const FDrawParams &p, const FVisualConfig &v,
    const FVector4 &base) const
{
    FVector4 c = base;
    c.w *= p.alphaMul;

    // Active wins over hover
    if (h != EHandle::None && h == p.activeHandle)
    {
        // Blend base->activeColor (activeBlend usually 1.0)
        const FVector4 target = v.activeColor;

        c = LerpRGB(c, target, std::clamp(v.activeBlend, 0.0f, 1.0f));

        // Alpha: base alpha * global alpha * activeAlphaMul * target alpha
        c.w = std::min(1.0f, c.w * v.activeAlphaMul * target.w);
        return c;
    }

    if (h != EHandle::None && h == p.hoveredHandle)
    {
        // Tint the handle color itself toward white
        const FVector4 white(1, 1, 1, 1);
        c = LerpRGB(c, white, std::clamp(v.hoverToWhite, 0.0f, 1.0f));

        // Hollow feeling: reduce alpha (should be < 1)
        c.w = std::min(1.0f, c.w * v.hoverAlphaMul);
        return c;
    }

    return c;
}

FDebugDrawStyle GizmoEditorTool::ApplyHandleStyle(EHandle h, const FDrawParams &p, const FVisualConfig &v,
                                                  FDebugDrawStyle s) const
{
    if (h != EHandle::None && h == p.hoveredHandle)
    {
        s.thicknessPx += v.hoverThickAddPx;
    }
    return s;
}

void GizmoEditorTool::BuildBasis(const FTransform& xf, ESpace space, // TODO: Make this adjust to the camera and make it always face the cam
                                 FVector3& outX, FVector3& outY, FVector3& outZ)
{
    // LH: +X forward, +Y right, +Z up
    const FQuat q = (space == ESpace::Local) ? xf.GetRotation() : FQuat{};
    const FVector3 AX = FVector3::Forward(); // "X axis" in your engine meaning
    const FVector3 AY = FVector3::Right();   // "Y axis"
    const FVector3 AZ = FVector3::Up();      // "Z axis"

    outX = q.RotateVector(AX).Normalized();
    outY = q.RotateVector(AY).Normalized();
    outZ = q.RotateVector(AZ).Normalized();
}

void GizmoEditorTool::DrawPlaneSquareWire(DebugDraw& dd, const FVector3& origin, const FVector3& axisA, const FVector3& axisB,
                                          float halfSize, const FVector4& color, const FDebugDrawStyle& style)
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

void GizmoEditorTool::DrawPlaneSquareSolid(DebugDraw &dd, const FVector3 &origin, const FVector3 &axisA,
    const FVector3 &axisB, float halfSize, const FVector4 &color, FDebugDrawStyle style)
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

// ----------------------
// HitId mapping
// ----------------------

uint32_t GizmoEditorTool::HandleToHitId(uint32_t baseHitId, EHandle h) const
{
    if (baseHitId == 0) return 0;

    // Keep groups separated to avoid confusion when debugging.
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

        // Scale (base + 20..23)
        case EHandle::S_X:      return baseHitId + 20;
        case EHandle::S_Y:      return baseHitId + 21;
        case EHandle::S_Z:      return baseHitId + 22;
        case EHandle::S_XY:      return baseHitId + 23;
        case EHandle::S_XZ:      return baseHitId + 24;
        case EHandle::S_YZ:      return baseHitId + 25;
        case EHandle::S_Uniform:return baseHitId + 26;

        default: return 0;
    }
}

GizmoEditorTool::EHandle GizmoEditorTool::HitIdToHandle(uint32_t baseHitId, uint32_t hitId) const
{
    if (baseHitId == 0 || hitId == 0) return EHandle::None;

    const uint32_t d = hitId - baseHitId;

    switch (d)
    {
        // Translate
        case 1: return EHandle::T_X;
        case 2: return EHandle::T_Y;
        case 3: return EHandle::T_Z;
        case 4: return EHandle::T_XY;
        case 5: return EHandle::T_XZ;
        case 6: return EHandle::T_YZ;
        case 7: return EHandle::T_Center;

        // Rotate
        case 10: return EHandle::R_X;
        case 11: return EHandle::R_Y;
        case 12: return EHandle::R_Z;
        case 13: return EHandle::R_Free;

        // Scale
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

void GizmoEditorTool::DrawTranslate(DebugDraw& dd, uint32_t viewKey, const FVector3& camPos, const FVector3& camFwd, const FTransform& xf,
                                    const FDrawParams& p, float gizmoScale, const FVisualConfig& v) const
{
    (void)camPos; (void)camFwd;

    FVector3 X, Y, Z;
    BuildBasis(xf, p.space, X, Y, Z);

    const FVector3 o = xf.GetPosition();
    const float axisLen   = v.axisLen    * gizmoScale;
    const float headLen   = v.headLen    * gizmoScale;
    const float headRad   = v.headRadius * gizmoScale;
    const float centerRad = v.centerRadius * gizmoScale;

    // Base style
    FDebugDrawStyle s{};
    s.layer = EDebugDrawLayer::Editor;
    s.depth = EDebugDepthMode::Overlay;
    s.fill = EDebugFillMode::Solid;
    s.thicknessPx = v.axisThicknessPx;
    s.shading = v.shading;
    s.normalMode = v.normalMode;
    s.viewKey = viewKey;

    // Colors
    FVector4 cx(0.85f, 0.10f, 0.10f, 1);
    FVector4 cy(0.10f, 0.85f, 0.10f, 1);
    FVector4 cz(0.10f, 0.10f, 0.85f, 1);
    FVector4 cCenter(0.85f, 0.85f, 0.85f, 1.0f);

    // Axis arrows

    // X
    {
        FDebugDrawStyle ps = s;
        ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_X);
        ps = ApplyHandleStyle(EHandle::T_X, p, v, ps);

        const FVector4 col = ApplyHandleTint(EHandle::T_X, p, v, cx);
        dd.DrawArrow(o, o + X * axisLen, col, headLen, headRad, 16, ps);
    }

    // Y
    {
        FDebugDrawStyle ps = s;
        ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_Y);
        ps = ApplyHandleStyle(EHandle::T_Y, p, v, ps);

        const FVector4 col = ApplyHandleTint(EHandle::T_Y, p, v, cy);
        dd.DrawArrow(o, o + Y * axisLen, col, headLen, headRad, 16, ps);
    }

    // Z
    {
        FDebugDrawStyle ps = s;
        ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_Z);
        ps = ApplyHandleStyle(EHandle::T_Z, p, v, ps);

        const FVector4 col = ApplyHandleTint(EHandle::T_Z, p, v, cz);
        dd.DrawArrow(o, o + Z * axisLen, col, headLen, headRad, 16, ps);
    }


    // Center sphere (free move)
    if (p.bDrawCenter)
    {
        FDebugDrawStyle cs = s;
        cs.thicknessPx = 1.0f;
        cs.hitId = HandleToHitId(p.baseHitID, EHandle::T_Center);
        cs = ApplyHandleStyle(EHandle::T_Center, p, v, cs);

        const FVector4 col = ApplyHandleTint(EHandle::T_Center, p, v, cCenter);
        dd.DrawSphere(o, centerRad, col, 24, cs);
    }

    // Plane handles (wire squares)
    if (p.bDrawPlanes)
    {
        FVector4 cxy(0, 0, 1, v.planeAlpha);
        FVector4 cxz(0, 1, 0, v.planeAlpha);
        FVector4 cyz(1, 0, 0, v.planeAlpha);

        const float half = (v.planeSize * 0.5f) * gizmoScale;
        const float off  = v.planeOffset * gizmoScale;

        // XY
        {
            FDebugDrawStyle ps = s;
            ps.thicknessPx = 2.0f;
            ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_XY);
            ps = ApplyHandleStyle(EHandle::T_XY, p, v, ps);

            const FVector3 c = o + (X + Y) * off;
            const FVector4 col = ApplyHandleTint(EHandle::T_XY, p, v, cxy);

            DrawPlaneSquareWire(dd, c, X, Y, half, col, ps);
            DrawPlaneSquareSolid(dd, c, X, Y, half, col, ps);
        }

        // XZ
        {
            FDebugDrawStyle ps = s;
            ps.thicknessPx = 2.0f;
            ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_XZ);
            ps = ApplyHandleStyle(EHandle::T_XZ, p, v, ps);

            const FVector3 c = o + (X + Z) * off;
            const FVector4 col = ApplyHandleTint(EHandle::T_XZ, p, v, cxz);

            DrawPlaneSquareWire(dd, c, X, Z, half, col, ps);
            DrawPlaneSquareSolid(dd, c, X, Z, half, col, ps);
        }

        // YZ
        {
            FDebugDrawStyle ps = s;
            ps.thicknessPx = 2.0f;
            ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_YZ);
            ps = ApplyHandleStyle(EHandle::T_YZ, p, v, ps);

            const FVector3 c = o + (Y + Z) * off;
            const FVector4 col = ApplyHandleTint(EHandle::T_YZ, p, v, cyz);

            DrawPlaneSquareWire(dd, c, Y, Z, half, col, ps);
            DrawPlaneSquareSolid(dd, c, Y, Z, half, col, ps);
        }
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

    // Base style
    FDebugDrawStyle s{};
    s.layer = EDebugDrawLayer::Editor;
    s.depth = EDebugDepthMode::Overlay;
    s.fill  = EDebugFillMode::Wireframe;
    s.thicknessPx = v.ringThicknessPx;
    s.shading = v.shading;
    s.normalMode = v.normalMode;
    s.viewKey = viewKey;

    FVector4 cx(0.85f, 0.10f, 0.10f, 1);
    FVector4 cy(0.10f, 0.85f, 0.10f, 1);
    FVector4 cz(0.10f, 0.10f, 0.85f, 1);

    // Optional faint sphere hint
    if (p.bDrawSphereHint)
    {
        FDebugDrawStyle hs = s;
        hs.thicknessPx = 1.0f;
        hs.hitId = 0; // not pickable
        hs.fill = EDebugFillMode::Solid;

        FVector4 hint(0.9f, 0.9f, 0.9f, v.sphereHintAlpha);
        dd.DrawSphere(o, r, hint, v.ringSegments, hs);
    }

    // X ring
    {
        FDebugDrawStyle rs = s;
        rs.hitId = HandleToHitId(p.baseHitID, EHandle::R_X);
        rs = ApplyHandleStyle(EHandle::R_X, p, v, rs);

        const FVector4 col = ApplyHandleTint(EHandle::R_X, p, v, cx);
        dd.DrawCircle(o, X, r, col, v.ringSegments, rs);
    }


    // Y ring
    {
        FDebugDrawStyle rs = s;
        rs.hitId = HandleToHitId(p.baseHitID, EHandle::R_Y);
        rs = ApplyHandleStyle(EHandle::R_Y, p, v, rs);

        const FVector4 col = ApplyHandleTint(EHandle::R_Y, p, v, cy);
        dd.DrawCircle(o, Y, r, col, v.ringSegments, rs);
    }

    // Z ring
    {
        FDebugDrawStyle rs = s;
        rs.hitId = HandleToHitId(p.baseHitID, EHandle::R_Z);
        rs = ApplyHandleStyle(EHandle::R_Z, p, v, rs);

        const FVector4 col = ApplyHandleTint(EHandle::R_Z, p, v, cz);
        dd.DrawCircle(o, Z, r, col, v.ringSegments, rs);
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

    // Styles
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

    FVector4 cx(0.85f, 0.10f, 0.10f, 1);
    FVector4 cy(0.10f, 0.85f, 0.10f, 1);
    FVector4 cz(0.10f, 0.10f, 0.85f, 1);
    FVector4 cU(0.9f,0.9f,0.9f,0.85f);

    const FVector3 ex = o + X * axisLen;
    const FVector3 ey = o + Y * axisLen;
    const FVector3 ez = o + Z * axisLen;

    // X
    {
        // Arm
        FDebugDrawStyle ls = sLine;
        ls.hitId = HandleToHitId(p.baseHitID, EHandle::S_X);
        ls = ApplyHandleStyle(EHandle::S_X, p, v, ls);

        const FVector4 colL = ApplyHandleTint(EHandle::S_X, p, v, cx);
        dd.DrawLine(o, ex, colL, ls);

        // Head
        FDebugDrawStyle bs = sBox;
        bs.hitId = HandleToHitId(p.baseHitID, EHandle::S_X);
        bs = ApplyHandleStyle(EHandle::S_X, p, v, bs);

        const FVector4 colB = ApplyHandleTint(EHandle::S_X, p, v, cx);
        const float h = v.scaleBoxHalf * gizmoScale;
        dd.DrawBox(ex, FVector3(h,h,h), colB, bs);
    }

    // Y
    {
        // Arm
        FDebugDrawStyle ls = sLine;
        ls.hitId = HandleToHitId(p.baseHitID, EHandle::S_Y);
        ls = ApplyHandleStyle(EHandle::S_Y, p, v, ls);

        const FVector4 colL = ApplyHandleTint(EHandle::S_Y, p, v, cy);
        dd.DrawLine(o, ey, colL, ls);

        // Head
        FDebugDrawStyle bs = sBox;
        bs.hitId = HandleToHitId(p.baseHitID, EHandle::S_Y);
        bs = ApplyHandleStyle(EHandle::S_Y, p, v, bs);

        const FVector4 colB = ApplyHandleTint(EHandle::S_Y, p, v, cy);
        const float h = v.scaleBoxHalf * gizmoScale;
        dd.DrawBox(ey, FVector3(h,h,h), colB, bs);
    }

    // Z
    {
        // Arm
        FDebugDrawStyle ls = sLine;
        ls.hitId = HandleToHitId(p.baseHitID, EHandle::S_Z);
        ls = ApplyHandleStyle(EHandle::S_Z, p, v, ls);

        const FVector4 colL = ApplyHandleTint(EHandle::S_Z, p, v, cz);
        dd.DrawLine(o, ez, colL, ls);

        // Head
        FDebugDrawStyle bs = sBox;
        bs.hitId = HandleToHitId(p.baseHitID, EHandle::S_Z);
        bs = ApplyHandleStyle(EHandle::S_Z, p, v, bs);

        const FVector4 colB = ApplyHandleTint(EHandle::S_Z, p, v, cz);
        const float h = v.scaleBoxHalf * gizmoScale;
        dd.DrawBox(ez, FVector3(h,h,h), colB, bs);
    }

    // Uniform center
    if (p.bDrawCenter)
    {
        FDebugDrawStyle bs = sBox;
        bs.hitId = HandleToHitId(p.baseHitID, EHandle::S_Uniform);
        bs = ApplyHandleStyle(EHandle::S_Uniform, p, v, bs);

        const FVector4 colB = ApplyHandleTint(EHandle::S_Uniform, p, v, cU);
        const float h = v.uniformBoxHalf * gizmoScale;
        dd.DrawBox(o, FVector3(h,h,h), colB, bs);
    }

    // Plane handles (wire squares)
    if (p.bDrawPlanes)
    {
        FVector4 cxy(0.10f, 0.10f, 0.85f, v.planeAlpha);
        FVector4 cxz(0.10f, 0.85f, 0.10f, v.planeAlpha);
        FVector4 cyz(0.85f, 0.10f, 0.10f, v.planeAlpha);

        const float off = v.scaleBiAxisLineScale * gizmoScale;

        sBox.thicknessPx = 2.f;

        // XY
        {
            FDebugDrawStyle ps = sBox;
            ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_XY);
            ps = ApplyHandleStyle(EHandle::T_XY, p, v, ps);

            const FVector4 col = ApplyHandleTint(EHandle::T_XY, p, v, cxy);

            dd.DrawLine(o + X*off, o + Y*off, col, ps);
        }

        // XZ
        {
            FDebugDrawStyle ps = sBox;
            ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_XZ);
            ps = ApplyHandleStyle(EHandle::T_XZ, p, v, ps);

            const FVector4 col = ApplyHandleTint(EHandle::T_XZ, p, v, cxz);

            dd.DrawLine(o + X*off, o + Z*off, col, ps);
        }

        // YZ
        {
            FDebugDrawStyle ps = sBox;
            ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_YZ);
            ps = ApplyHandleStyle(EHandle::T_YZ, p, v, ps);

            const FVector4 col = ApplyHandleTint(EHandle::T_YZ, p, v, cyz);

            dd.DrawLine(o + Y*off, o + Z*off, col, ps);
        }
    }
}