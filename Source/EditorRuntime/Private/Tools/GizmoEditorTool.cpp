//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Tools/GizmoEditorTool.h"

#include <algorithm>
#include <cmath>

FVector4 GizmoEditorTool::Brighten(const FVector4& c, float mul)
{
    return FVector4(
        std::min(1.0f, c.x * mul),
        std::min(1.0f, c.y * mul),
        std::min(1.0f, c.z * mul),
        c.w
    );
}

float GizmoEditorTool::ComputeGizmoScale(const FVector3& camPos, const FVector3& gizmoPos)
{
    // Simple constant-size heuristic.
    // Tune these later; feels decent in practice.
    const float dist = (gizmoPos - camPos).Length();
    return std::max(0.3f, dist * 0.10f);
}

void GizmoEditorTool::BuildBasis(const FTransform& xf, ESpace space,
                                 FVector3& outX, FVector3& outY, FVector3& outZ)
{
    // LH: +X forward, +Y right, +Z up
    const FQuat q = (space == ESpace::Local) ? xf.GetRotation() : FQuat{};
    outX = q.RotateVector(FVector3(1,0,0)).Normalized();
    outY = q.RotateVector(FVector3(0,1,0)).Normalized();
    outZ = q.RotateVector(FVector3(0,0,1)).Normalized();
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
        case EHandle::S_Uniform:return baseHitId + 23;

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
        case 23: return EHandle::S_Uniform;

        default: return EHandle::None;
    }
}

// ----------------------
// Draw entry
// ----------------------

void GizmoEditorTool::Draw(DebugDraw& dd, uint32_t viewKey, const FVector3& camPos, const FVector3& camFwd,
                           const FTransform& gizmoXf, const FDrawParams& p) const
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
    s.viewKey = viewKey;

    // Colors
    FVector4 cx(1,0,0,1);
    FVector4 cy(0,1,0,1);
    FVector4 cz(0,0,1,1);
    FVector4 cCenter(0.85f, 0.85f, 0.85f, 1.0f);

    auto applyHL = [&](EHandle h, FVector4 col) -> FVector4
    {
        if (h == p.activeHandle)  return Brighten(col, v.activeMul);
        if (h == p.hoveredHandle) return Brighten(col, v.highlightMul);
        return col;
    };

    // Axis arrows
    s.hitId = HandleToHitId(p.baseHitID, EHandle::T_X);
    dd.DrawArrow(o, o + X * axisLen, applyHL(EHandle::T_X, cx), headLen, headRad, 16, s);

    s.hitId = HandleToHitId(p.baseHitID, EHandle::T_Y);
    dd.DrawArrow(o, o + Y * axisLen, applyHL(EHandle::T_Y, cy), headLen, headRad, 16, s);

    s.hitId = HandleToHitId(p.baseHitID, EHandle::T_Z);
    dd.DrawArrow(o, o + Z * axisLen, applyHL(EHandle::T_Z, cz), headLen, headRad, 16, s);

    // Center sphere (free move)
    if (p.bDrawCenter)
    {
        FDebugDrawStyle cs = s;
        cs.thicknessPx = 1.0f;
        cs.hitId = HandleToHitId(p.baseHitID, EHandle::T_Center);

        FVector4 col = applyHL(EHandle::T_Center, cCenter);
        dd.DrawSphere(o, centerRad, col, 24, cs);
    }

    // Plane handles (wire squares)
    if (p.bDrawPlanes)
    {
        // Make planes slightly transparent
        FVector4 cxy = FVector4(0, 0, 1, v.planeAlpha);  // XY: blue
        FVector4 cxz = FVector4(0, 1, 0, v.planeAlpha);  // XZ: green
        FVector4 cyz = FVector4(1, 0, 0, v.planeAlpha);  // YZ: red

        const float half = (v.planeSize * 0.5f) * gizmoScale;
        const float off  = v.planeOffset * gizmoScale;

        // XY plane square (axes X,Y), offset away from origin to avoid clutter
        {
            FDebugDrawStyle ps = s;
            ps.thicknessPx = 2.0f;
            ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_XY);

            const FVector3 c = o + (X + Y) * off;
            const FVector4 col = applyHL(EHandle::T_XY, cxy);
            DrawPlaneSquareSolid(dd, c, X, Y, half, col, ps);
        }

        // XZ
        {
            FDebugDrawStyle ps = s;
            ps.thicknessPx = 2.0f;
            ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_XZ);

            const FVector3 c = o + (X + Z) * off;
            const FVector4 col = applyHL(EHandle::T_XZ, cxz);
            DrawPlaneSquareSolid(dd, c, X, Z, half, col, ps);
        }

        // YZ
        {
            FDebugDrawStyle ps = s;
            ps.thicknessPx = 2.0f;
            ps.hitId = HandleToHitId(p.baseHitID, EHandle::T_YZ);

            const FVector3 c = o + (Y + Z) * off;
            const FVector4 col = applyHL(EHandle::T_YZ, cyz);
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
    s.viewKey = viewKey;

    FVector4 cx(1,0,0,1);
    FVector4 cy(0,1,0,1);
    FVector4 cz(0,0,1,1);

    auto applyHL = [&](EHandle h, FVector4 col) -> FVector4
    {
        if (h == p.activeHandle)  return Brighten(col, v.activeMul);
        if (h == p.hoveredHandle) return Brighten(col, v.highlightMul);
        return col;
    };

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

    // X ring (normal = X axis => circle lies in YZ)
    s.hitId = HandleToHitId(p.baseHitID, EHandle::R_X);
    dd.DrawCircle(o, X, r, applyHL(EHandle::R_X, cx), v.ringSegments, s);

    // Y ring
    s.hitId = HandleToHitId(p.baseHitID, EHandle::R_Y);
    dd.DrawCircle(o, Y, r, applyHL(EHandle::R_Y, cy), v.ringSegments, s);

    // Z ring
    s.hitId = HandleToHitId(p.baseHitID, EHandle::R_Z);
    dd.DrawCircle(o, Z, r, applyHL(EHandle::R_Z, cz), v.ringSegments, s);

    // Optional free-rotate handle (center sphere)
    // (If you want: treat as R_Free hitId, otherwise skip)
    // If you enable it, it’ll be hit-testable via line hits only right now (sphere wireframe).
    // Later, draw it solid for tri picking.
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
    sLine.viewKey = viewKey;

    FDebugDrawStyle sBox = sLine;
    sBox.fill = EDebugFillMode::Solid;
    sBox.thicknessPx = 1.0f;

    FVector4 cx(1,0,0,1);
    FVector4 cy(0,1,0,1);
    FVector4 cz(0,0,1,1);
    FVector4 cU(0.9f,0.9f,0.9f,1);

    auto applyHL = [&](EHandle h, FVector4 col) -> FVector4
    {
        if (h == p.activeHandle)  return Brighten(col, v.activeMul);
        if (h == p.hoveredHandle) return Brighten(col, v.highlightMul);
        return col;
    };

    const FVector3 ex = o + X * axisLen;
    const FVector3 ey = o + Y * axisLen;
    const FVector3 ez = o + Z * axisLen;

    // Arms (lines)
    sLine.hitId = HandleToHitId(p.baseHitID, EHandle::S_X);
    dd.DrawLine(o, ex, applyHL(EHandle::S_X, cx), sLine);

    sLine.hitId = HandleToHitId(p.baseHitID, EHandle::S_Y);
    dd.DrawLine(o, ey, applyHL(EHandle::S_Y, cy), sLine);

    sLine.hitId = HandleToHitId(p.baseHitID, EHandle::S_Z);
    dd.DrawLine(o, ez, applyHL(EHandle::S_Z, cz), sLine);

    // Heads (boxes)
    {
        sBox.hitId = HandleToHitId(p.baseHitID, EHandle::S_X);
        const float h = v.scaleBoxHalf * gizmoScale;
        dd.DrawBox(ex, FVector3(h,h,h), applyHL(EHandle::S_X, cx), sBox);
    }
    {
        sBox.hitId = HandleToHitId(p.baseHitID, EHandle::S_Y);
        const float h = v.scaleBoxHalf * gizmoScale;
        dd.DrawBox(ey, FVector3(h,h,h), applyHL(EHandle::S_Y, cy), sBox);
    }
    {
        sBox.hitId = HandleToHitId(p.baseHitID, EHandle::S_Z);
        const float h = v.scaleBoxHalf * gizmoScale;
        dd.DrawBox(ez, FVector3(h,h,h), applyHL(EHandle::S_Z, cz), sBox);
    }

    // Center uniform scale box
    if (p.bDrawCenter)
    {
        sBox.hitId = HandleToHitId(p.baseHitID, EHandle::S_Uniform);
        const float h = v.uniformBoxHalf * gizmoScale;
        dd.DrawBox(o, FVector3(h,h,h), applyHL(EHandle::S_Uniform, cU), sBox);
    }
}
