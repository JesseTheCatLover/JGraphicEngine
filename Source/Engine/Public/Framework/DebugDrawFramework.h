//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

#include "Core/Math/FVector3.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FMath.h"

using namespace FMath;

class IRenderDevice;
struct FRenderView;

enum class EDebugFillMode : uint8_t
{
    Wireframe = 0,
    Solid     = 1
};

enum class EDebugDepthMode : uint8_t
{
    Overlay,   // depth test OFF
    DepthTest  // depth test ON
};

enum class EDebugShading : uint8_t
{
    Unlit,       // Unlit
    FixedLit,   // fake “editor light” look
};

enum class EDebugNormalMode : uint8_t { Flat, Smooth };

enum class EDebugDrawLayer : uint8_t
{
    Gameplay = 0,
    Editor   = 1,
    Physics  = 2,
    AI       = 3,
    Nav      = 4,
    Custom0  = 5,
    Custom1  = 6,
    Custom2  = 7,
    Count
};

// Single style for everything.
// Note: For lines, `fill` is ignored.
struct FDebugDrawStyle
{
    float thicknessPx = 1.0f; // <= 1 = thin line, >1 = quad in clip space
    EDebugFillMode  fill  = EDebugFillMode::Wireframe;
    EDebugDepthMode depth = EDebugDepthMode::Overlay;
    EDebugDrawLayer layer = EDebugDrawLayer::Gameplay;

    uint32_t hitId = 0; // 0 = not pickable
    EDebugShading shading = EDebugShading::Unlit;
    EDebugNormalMode normalMode = EDebugNormalMode::Flat;
    uint32_t viewKey = 0;  // 0 = draw in ALL views; otherwise only if matches view.debugViewKey
};

struct FDebugLine
{
    FVector3 a;
    FVector3 b;
    FVector4 color; // linear RGBA
    FDebugDrawStyle style;
};

struct FDebugTri
{
    FVector3 a, b, c;
    FVector3 na, nb, nc;
    FVector4 color; // linear RGBA
    FDebugDrawStyle style;
};

struct FDebugVertex
{
    float x, y, z;
    float r, g, b, a;
};

struct FDebugClipVertex
{
    float x, y, z, w; // clip-space position
    float r, g, b, a;
};

struct FDebugWorldVertex
{
    float x, y, z;
    float nx, ny, nz;
    float r, g, b, a;
};

struct FDebugHit
{
    enum class EType : uint8_t { None, Line, Tri };

    EType type = EType::None;
    uint32_t hitId = 0;        // stable ID you set when drawing
    uint32_t index = 0;        // index in the tested list (optional / debug)
    float depth01 = 1.0f;    // 0 near .. 1 far (best-effort)
    float distPx = 1e9f;    // for lines/overlay tie-break
    float tRay = 1e30f;   // for ray hits (smaller = closer)

    FVector3 worldPos = FVector3(0,0,0);
};

struct FScreenPt
{
    float x, y;     // pixels
    float depth01;  // 0..1
    bool  valid;
};

class DebugDraw
{
private:
    struct FTimedLine
    {
        FDebugLine line;
        float timeRemaining = 0.0f;
    };
    struct FTimedTri
    {
        FDebugTri tri;
        float timeRemaining = 0.0f;
    };

    bool     m_bEnabled   = true;
    uint32_t m_LayerMask = 0xFFFFFFFFu;

    std::vector<FDebugLine> m_Immediate;
    std::vector<FTimedLine> m_Timed;

    std::vector<FDebugTri>  m_ImmediateTris;
    std::vector<FTimedTri>  m_TimedTris;

private:
    static constexpr uint32_t DebugLayerBit(EDebugDrawLayer layer)
    {
        return 1u << uint32_t(layer);
    }

    static bool WantSmoothLitSolid(const FDebugDrawStyle& style)
    {
        return style.shading == EDebugShading::FixedLit &&
               style.fill    == EDebugFillMode::Solid &&
               style.normalMode == EDebugNormalMode::Smooth;
    };

    [[nodiscard]] bool PassLayer(EDebugDrawLayer layer) const
    {
        return (m_LayerMask & DebugLayerBit(layer)) != 0;
    }

    void EmitLineInternal(std::vector<FDebugLine>& dst,
                          const FVector3& a, const FVector3& b,
                          const FVector4& c, const FDebugDrawStyle& s);

    void EmitTriInternal(std::vector<FDebugTri>& dst,
                         const FVector3& a, const FVector3& b, const FVector3& c,
                         const FVector4& col, const FDebugDrawStyle& s);

    void EmitTriInternalN(std::vector<FDebugTri>& dst,
                      const FVector3& a, const FVector3& b, const FVector3& c,
                      const FVector3& na, const FVector3& nb, const FVector3& nc,
                      const FVector4& col, const FDebugDrawStyle& s);

    void EmitLineTimedInternal(const FVector3& a, const FVector3& b,
                               const FVector4& c, const FDebugDrawStyle& s,
                               float seconds);

    void EmitTriTimedInternal(const FVector3& a, const FVector3& b, const FVector3& c,
                              const FVector4& col, const FDebugDrawStyle& s,
                              float seconds);

    void EmitTriTimedInternalN(const FVector3& a, const FVector3& b, const FVector3& c,
                                      const FVector3& na, const FVector3& nb, const FVector3& nc,
                                      const FVector4& col, const FDebugDrawStyle& s,
                                      float seconds);

    static float DistPointToSegment2D(float px, float py,
                                      float ax, float ay,
                                      float bx, float by);

    static bool BuildRayFromMouse(const FRenderView& view,
                                  const FMatrix4& invVP,
                                  float mouseX_px,
                                  float mouseY_px,
                                  FVector3& outOrigin,
                                  FVector3& outDir);

    static bool RayTriIntersectMT(const FVector3& ro, const FVector3& rd,
                                  const FVector3& a, const FVector3& b, const FVector3& c,
                                  float& outT, float& outU, float& outV);

private:
    // ---------- Geometry builders (HEADER-ONLY templates) ----------

    static void BuildBoxCorners(const FVector3& c, const FVector3& e, FVector3 out[8])
    {
        out[0] = c + FVector3(-e.x, -e.y, -e.z);
        out[1] = c + FVector3( e.x, -e.y, -e.z);
        out[2] = c + FVector3( e.x,  e.y, -e.z);
        out[3] = c + FVector3(-e.x,  e.y, -e.z);
        out[4] = c + FVector3(-e.x, -e.y,  e.z);
        out[5] = c + FVector3( e.x, -e.y,  e.z);
        out[6] = c + FVector3( e.x,  e.y,  e.z);
        out[7] = c + FVector3(-e.x,  e.y,  e.z);
    }

    template <typename EmitLineFn, typename EmitTriFn>
    static void BuildCircleGeom(const FVector3& center, const FVector3& normal, float radius,
                                int segments, EDebugFillMode fill,
                                EmitLineFn&& emitLine, EmitTriFn&& emitTri)
    {
        if (segments < 6) segments = 6;
        if (radius <= 1e-6f) return;

        FVector3 n = NormalizeSafe(normal);
        if (n.Length() <= 0.0f) return;

        FVector3 t = (std::fabs(n.z) < 0.999f)
            ? FVector3(0,0,1).Cross(n)
            : FVector3(0,1,0).Cross(n);
        t = NormalizeSafe(t);
        const FVector3 b = n.Cross(t);

        const float step = (2.0f * 3.1415926535f) / float(segments);

        std::vector<FVector3> ring;
        ring.reserve((size_t)segments);

        for (int i = 0; i < segments; ++i)
        {
            const float a = step * float(i);
            ring.push_back(center + (t * std::cos(a) + b * std::sin(a)) * radius);
        }

        if (fill == EDebugFillMode::Wireframe)
        {
            for (int i = 0; i < segments; ++i)
            {
                const int j = (i + 1) % segments;
                emitLine(ring[i], ring[j]);
            }
        }
        else
        {
            // solid disk
            for (int i = 0; i < segments; ++i)
            {
                const int j = (i + 1) % segments;
                emitTri(center, ring[j], ring[i]);
            }
        }
    }

    template <typename EmitLineFn, typename EmitTriFn>
    static void BuildSphereGeom(const FVector3& center, float radius,
                                int segments, EDebugFillMode fill,
                                EmitLineFn&& emitLine, EmitTriFn&& emitTri)
    {
        if (radius <= 1e-6f) return;
        segments = std::max(segments, 6);

        if (fill == EDebugFillMode::Wireframe)
        {
            BuildCircleGeom(center, FVector3(1,0,0), radius, segments, EDebugFillMode::Wireframe, emitLine, emitTri);
            BuildCircleGeom(center, FVector3(0,1,0), radius, segments, EDebugFillMode::Wireframe, emitLine, emitTri);
            BuildCircleGeom(center, FVector3(0,0,1), radius, segments, EDebugFillMode::Wireframe, emitLine, emitTri);
            return;
        }

        // solid UV sphere
        const int slices = std::max(segments, 8);
        const int stacks = std::max(segments / 2, 4);

        for (int y = 0; y < stacks; ++y)
        {
            const float v0 = float(y)     / float(stacks);
            const float v1 = float(y + 1) / float(stacks);

            const float phi0 = v0 * 3.1415926535f;
            const float phi1 = v1 * 3.1415926535f;

            const float z0 = std::cos(phi0);
            const float z1 = std::cos(phi1);

            const float r0 = std::sin(phi0);
            const float r1 = std::sin(phi1);

            for (int x = 0; x < slices; ++x)
            {
                const float u0 = float(x)     / float(slices);
                const float u1 = float(x + 1) / float(slices);

                const float th0 = u0 * (2.0f * 3.1415926535f);
                const float th1 = u1 * (2.0f * 3.1415926535f);

                const FVector3 p00 = center + FVector3(r0 * std::cos(th0), r0 * std::sin(th0), z0) * radius;
                const FVector3 p10 = center + FVector3(r0 * std::cos(th1), r0 * std::sin(th1), z0) * radius;
                const FVector3 p01 = center + FVector3(r1 * std::cos(th0), r1 * std::sin(th0), z1) * radius;
                const FVector3 p11 = center + FVector3(r1 * std::cos(th1), r1 * std::sin(th1), z1) * radius;

                emitTri(p00, p01, p11);
                emitTri(p00, p11, p10);
            }
        }
    }

    template <typename EmitLineFn, typename EmitTriFn>
    static void BuildBoxGeom(const FVector3& center, const FVector3& halfExtents,
                             EDebugFillMode fill, EmitLineFn&& emitLine, EmitTriFn&& emitTri)
    {
        FVector3 v[8];
        BuildBoxCorners(center, halfExtents, v);

        if (fill == EDebugFillMode::Wireframe)
        {
            auto L = [&](int a, int b){ emitLine(v[a], v[b]); };
            L(0,1); L(1,2); L(2,3); L(3,0);
            L(4,5); L(5,6); L(6,7); L(7,4);
            L(0,4); L(1,5); L(2,6); L(3,7);
            return;
        }

        auto T = [&](int a, int b, int c){ emitTri(v[a], v[b], v[c]); };

        T(0,1,2); T(0,2,3); // -Z
        T(4,6,5); T(4,7,6); // +Z
        T(0,5,1); T(0,4,5); // -Y
        T(3,2,6); T(3,6,7); // +Y
        T(0,3,7); T(0,7,4); // -X
        T(1,5,6); T(1,6,2); // +X
    }

    template <typename EmitLineFn, typename EmitTriFn>
    static void BuildConeGeom(const FVector3& apex, const FVector3& dir, float height, float radius,
                              int segments, EDebugFillMode fill,
                              EmitLineFn&& emitLine, EmitTriFn&& emitTri)
    {
        if (segments < 6) segments = 6;
        if (height <= 1e-6f || radius <= 1e-6f) return;

        const FVector3 n = NormalizeSafe(dir);
        if (n.Length() <= 0.0f) return;

        const FVector3 baseCenter = apex + n * height;

        FVector3 t = (std::fabs(n.z) < 0.999f)
            ? FVector3(0,0,1).Cross(n)
            : FVector3(0,1,0).Cross(n);
        t = NormalizeSafe(t);
        const FVector3 b = n.Cross(t);

        const float step = (2.0f * 3.1415926535f) / float(segments);

        std::vector<FVector3> ring;
        ring.reserve((size_t)segments);

        for (int i = 0; i < segments; ++i)
        {
            const float a = step * float(i);
            ring.push_back(baseCenter + (t * std::cos(a) + b * std::sin(a)) * radius);
        }

        if (fill == EDebugFillMode::Wireframe)
        {
            for (int i = 0; i < segments; ++i)
            {
                const int j = (i + 1) % segments;
                emitLine(ring[i], ring[j]);
                emitLine(apex, ring[i]);
            }
            return;
        }

        for (int i = 0; i < segments; ++i)
        {
            const int j = (i + 1) % segments;
            emitTri(apex, ring[i], ring[j]);
            emitTri(baseCenter, ring[j], ring[i]);
        }
    }

    template <typename EmitLineFn, typename EmitTriFn>
    static void BuildCylinderGeom(const FVector3& baseCenter, const FVector3& axisDir,
                                  float height, float radius, int segments,
                                  EDebugFillMode fill, EmitLineFn&& emitLine, EmitTriFn&& emitTri)
    {
        if (segments < 6) segments = 6;
        if (height <= 1e-6f || radius <= 1e-6f) return;

        const FVector3 n = NormalizeSafe(axisDir);
        if (n.Length() <= 0.0f) return;

        const FVector3 topCenter = baseCenter + n * height;

        FVector3 t = (std::fabs(n.z) < 0.999f) ? FVector3(0,0,1).Cross(n) : FVector3(0,1,0).Cross(n);
        t = NormalizeSafe(t);
        const FVector3 b = n.Cross(t);

        const float step = (2.0f * 3.1415926535f) / float(segments);

        std::vector<FVector3> ring0, ring1;
        ring0.reserve((size_t)segments);
        ring1.reserve((size_t)segments);

        for (int i = 0; i < segments; ++i)
        {
            const float a = step * float(i);
            const FVector3 off = (t * std::cos(a) + b * std::sin(a)) * radius;
            ring0.push_back(baseCenter + off);
            ring1.push_back(topCenter  + off);
        }

        if (fill == EDebugFillMode::Wireframe)
        {
            for (int i = 0; i < segments; ++i)
            {
                const int j = (i + 1) % segments;
                emitLine(ring0[i], ring0[j]);
                emitLine(ring1[i], ring1[j]);
                emitLine(ring0[i], ring1[i]);
            }
            return;
        }

        for (int i = 0; i < segments; ++i)
        {
            const int j = (i + 1) % segments;

            emitTri(ring0[i], ring1[j], ring1[i]);
            emitTri(ring0[i], ring0[j], ring1[j]);

            emitTri(baseCenter, ring0[j], ring0[i]);
            emitTri(topCenter,  ring1[i], ring1[j]);
        }
    }

    template <typename EmitLineFn, typename EmitTriFn>
    static void BuildArrowGeom(const FVector3& a, const FVector3& b,
                           float headLen, float headRadius, int headSegments,
                           EDebugFillMode fill,
                           EmitLineFn&& emitLine, EmitTriFn&& emitTri)
    {
        FVector3 dir = b - a;
        const float len = dir.Length();
        if (len <= 1e-6f) return;

        dir = dir / len;

        const float hl = std::min(headLen, len * 0.5f);
        const float shaftLen = std::max(0.0f, len - hl);

        // Wireframe = lines only, Solid = tris only
        if (fill == EDebugFillMode::Wireframe)
        {
            // shaft as a line
            emitLine(a, b);

            // head as wireframe cone
            BuildConeGeom(b, -dir, hl, headRadius, headSegments,
                          EDebugFillMode::Wireframe, emitLine, emitTri);
            return;
        }

        // Solid: shaft as a cylinder (tris), head as solid cone (tris)
        const float shaftRadius = headRadius * 0.18f;  // TODO: maybe should be tweakable

        if (shaftLen > 1e-6f && shaftRadius > 1e-6f)
        {
            BuildCylinderGeom(a, dir, shaftLen, shaftRadius,
                              std::max(6, headSegments),
                              EDebugFillMode::Solid, emitLine, emitTri);
        }

        BuildConeGeom(b, -dir, hl, headRadius, headSegments,
                      EDebugFillMode::Solid, emitLine, emitTri);
    }

    template <typename EmitLineFn, typename EmitTriFn>
    static void BuildCapsuleGeom(const FVector3& center, const FVector3& axisDir,
                                 float halfHeight, float radius, int segments,
                                 EDebugFillMode fill,
                                 EmitLineFn&& emitLine, EmitTriFn&& emitTri)
    {
        if (halfHeight <= 1e-6f || radius <= 1e-6f) return;

        const FVector3 n = NormalizeSafe(axisDir);
        if (n.Length() <= 0.0f) return;

        const FVector3 a = center - n * halfHeight;
        const FVector3 b = center + n * halfHeight;

        BuildCylinderGeom(a, n, halfHeight * 2.0f, radius, segments * 2, fill, emitLine, emitTri);
        BuildSphereGeom(a, radius, segments * 2, fill, emitLine, emitTri);
        BuildSphereGeom(b, radius, segments * 2, fill, emitLine, emitTri);
    }

    // ----- Draw Immediate/Timed Implementation templates -----

    template <bool bTimed>
    void DrawLineImpl(const FVector3& a, const FVector3& b, const FVector4& color,
                                 float seconds, const FDebugDrawStyle& style)
    {
        if constexpr (bTimed)
            EmitLineTimedInternal(a, b, color, style, seconds);
        else
            EmitLineInternal(m_Immediate, a, b, color, style);
    }

    template <bool bTimed>
    void DrawAxisTriadImpl(const FTransform& t, float scale,
                                      float seconds, const FDebugDrawStyle& style)
    {
        const FVector3 o = t.GetPosition();
        const FQuat q    = t.GetRotation();

        const FVector3 xAxis = q.RotateVector(FVector3(1, 0, 0));
        const FVector3 yAxis = q.RotateVector(FVector3(0, 1, 0));
        const FVector3 zAxis = q.RotateVector(FVector3(0, 0, 1));

        DrawLineImpl<bTimed>(o, o + xAxis * scale, FVector4(1, 0, 0, 1), seconds, style);
        DrawLineImpl<bTimed>(o, o + yAxis * scale, FVector4(0, 1, 0, 1), seconds, style);
        DrawLineImpl<bTimed>(o, o + zAxis * scale, FVector4(0, 0, 1, 1), seconds, style);
    }

    template <bool bTimed>
    void DrawCircleImpl(const FVector3& center, const FVector3& normal, float radius, const FVector4& color,
                                   float seconds, int segments, const FDebugDrawStyle& style)
    {
        auto emitL = [&](const FVector3& a, const FVector3& b)
        {
            DrawLineImpl<bTimed>(a, b, color, seconds, style);
        };

        auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
        {
            if constexpr (bTimed)
                EmitTriTimedInternal(a, b, c, color, style, seconds);
            else
                EmitTriInternal(m_ImmediateTris, a, b, c, color, style);
        };

        BuildCircleGeom(center, normal, radius, segments, style.fill, emitL, emitT);
    }

    template <bool bTimed>
    void DrawSphereImpl(const FVector3& center, float radius, const FVector4& color,
                                   float seconds, int segments, const FDebugDrawStyle& style)
    {
        auto emitL = [&](const FVector3& a, const FVector3& b)
        {
            DrawLineImpl<bTimed>(a, b, color, seconds, style);
        };

        auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
        {
            if (WantSmoothLitSolid(style))
            {
                const FVector3 na = NormalizeSafe(a - center);
                const FVector3 nb = NormalizeSafe(b - center);
                const FVector3 nc = NormalizeSafe(c - center);

                if constexpr (bTimed)
                    EmitTriTimedInternalN(a,b,c, na,nb,nc, color, style, seconds);
                else
                    EmitTriInternalN(m_ImmediateTris, a,b,c, na,nb,nc, color, style);
            }
            else
            {
                if constexpr (bTimed)
                    EmitTriTimedInternal(a,b,c, color, style, seconds);
                else
                    EmitTriInternal(m_ImmediateTris, a,b,c, color, style);
            }
        };

        BuildSphereGeom(center, radius, segments, style.fill, emitL, emitT);
    }

    template <bool bTimed>
    void DrawBoxImpl(const FVector3& center, const FVector3& halfExtents, const FVector4& color,
                                float seconds, const FDebugDrawStyle& style)
    {
        auto emitL = [&](const FVector3& a, const FVector3& b)
        {
            DrawLineImpl<bTimed>(a, b, color, seconds, style);
        };

        auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
        {
            if constexpr (bTimed)
                EmitTriTimedInternal(a, b, c, color, style, seconds);
            else
                EmitTriInternal(m_ImmediateTris, a, b, c, color, style);
        };

        BuildBoxGeom(center, halfExtents, style.fill, emitL, emitT);
    }

    template <bool bTimed>
    void DrawCylinderImpl(const FVector3& baseCenter, const FVector3& axisDir, float height, float radius,
                                     const FVector4& color, float seconds, int segments, const FDebugDrawStyle& style)
    {
        auto emitL = [&](const FVector3& a, const FVector3& b)
        {
            DrawLineImpl<bTimed>(a, b, color, seconds, style);
        };

        const FVector3 axis = NormalizeSafe(axisDir);

        auto cylNormal = [&](const FVector3& p) -> FVector3
        {
            const FVector3 v = p - baseCenter;
            const float t = v.Dot(axis);
            const FVector3 radial = v - axis * t;
            return NormalizeSafe(radial);
        };

        auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
        {
            if (WantSmoothLitSolid(style))
            {
                const FVector3 flat = NormalizeSafe((b - a).Cross(c - a));
                const float capness = std::fabs(flat.Dot(axis));

                if (capness > 0.85f)
                {
                    const float sign = (flat.Dot(axis) >= 0.0f) ? 1.0f : -1.0f;
                    const FVector3 n = axis * sign;

                    if constexpr (bTimed)
                        EmitTriTimedInternalN(a,b,c, n,n,n, color, style, seconds);
                    else
                        EmitTriInternalN(m_ImmediateTris, a,b,c, n,n,n, color, style);
                }
                else
                {
                    const FVector3 na = cylNormal(a);
                    const FVector3 nb = cylNormal(b);
                    const FVector3 nc = cylNormal(c);

                    if constexpr (bTimed)
                        EmitTriTimedInternalN(a,b,c, na,nb,nc, color, style, seconds);
                    else
                        EmitTriInternalN(m_ImmediateTris, a,b,c, na,nb,nc, color, style);
                }
            }
            else
            {
                if constexpr (bTimed)
                    EmitTriTimedInternal(a,b,c, color, style, seconds);
                else
                    EmitTriInternal(m_ImmediateTris, a,b,c, color, style);
            }
        };

        BuildCylinderGeom(baseCenter, axisDir, height, radius, segments, style.fill, emitL, emitT);
    }

    template <bool bTimed>
    void DrawConeImpl(const FVector3& apex, const FVector3& dir, float height, float radius,
                                 const FVector4& color, float seconds, int segments, const FDebugDrawStyle& style)
    {
        auto emitL = [&](const FVector3& a, const FVector3& b)
        {
            DrawLineImpl<bTimed>(a, b, color, seconds, style);
        };

        const FVector3 axis = NormalizeSafe(dir);
        const float k = (height > 1e-6f) ? (radius / height) : 0.0f;

        auto coneSideNormal = [&](const FVector3& p) -> FVector3
        {
            const FVector3 v = p - apex;
            const float t = v.Dot(axis);
            const FVector3 radial = v - axis * t;
            const FVector3 rhat = NormalizeSafe(radial);
            return NormalizeSafe(rhat - axis * k);
        };

        auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
        {
            if (WantSmoothLitSolid(style))
            {
                const FVector3 flat = NormalizeSafe((b - a).Cross(c - a));
                const float capness = std::fabs(flat.Dot(axis));

                if (capness > 0.85f)
                {
                    const float sign = (flat.Dot(axis) >= 0.0f) ? 1.0f : -1.0f;
                    const FVector3 n = axis * sign;

                    if constexpr (bTimed)
                        EmitTriTimedInternalN(a,b,c, n,n,n, color, style, seconds);
                    else
                        EmitTriInternalN(m_ImmediateTris, a,b,c, n,n,n, color, style);
                }
                else
                {
                    const FVector3 na = coneSideNormal(a);
                    const FVector3 nb = coneSideNormal(b);
                    const FVector3 nc = coneSideNormal(c);

                    if constexpr (bTimed)
                        EmitTriTimedInternalN(a,b,c, na,nb,nc, color, style, seconds);
                    else
                        EmitTriInternalN(m_ImmediateTris, a,b,c, na,nb,nc, color, style);
                }
            }
            else
            {
                if constexpr (bTimed)
                    EmitTriTimedInternal(a,b,c, color, style, seconds);
                else
                    EmitTriInternal(m_ImmediateTris, a,b,c, color, style);
            }
        };

        BuildConeGeom(apex, dir, height, radius, segments, style.fill, emitL, emitT);
    }

    template <bool bTimed>
    void DrawArrowImpl(const FVector3& a, const FVector3& b, const FVector4& color,
                                  float seconds, float headLen, float headRadius, int headSegments,
                                  const FDebugDrawStyle& style)
    {
        auto emitL = [&](const FVector3& p0, const FVector3& p1)
        {
            DrawLineImpl<bTimed>(p0, p1, color, seconds, style);
        };

        const FVector3 ab = b - a;
        const float len = ab.Length();
        if (len <= 1e-6f)
            return;

        const FVector3 axis = ab / len;

        const float hl = std::min(headLen, len * 0.5f);
        const float headStartT = len - hl;
        const FVector3 headBase = b - axis * hl;

        auto shaftNormal = [&](const FVector3& p) -> FVector3
        {
            const float t = (p - a).Dot(axis);
            const FVector3 closest = a + axis * t;
            return NormalizeSafe(p - closest);
        };

        const FVector3 coneDir = NormalizeSafe(headBase - b); // ~ -axis
        const float k = (hl > 1e-6f) ? (headRadius / hl) : 0.0f;

        auto coneSideNormal = [&](const FVector3& p) -> FVector3
        {
            const FVector3 v = p - b;
            const float t = v.Dot(coneDir);
            const FVector3 radial = v - coneDir * t;
            const FVector3 rhat = NormalizeSafe(radial);
            return NormalizeSafe(rhat - coneDir * k);
        };

        auto faceNormal = [&](const FVector3& p0, const FVector3& p1, const FVector3& p2) -> FVector3
        {
            return NormalizeSafe((p1 - p0).Cross(p2 - p0));
        };

        const FVector3 apexN = NormalizeSafe(-coneDir); // stable apex normal (prevents "star tip")

        auto isSamePoint = [&](const FVector3& p, const FVector3& q) -> bool
        {
            const FVector3 d = p - q;
            return d.Dot(d) < 1e-10f; // epsilon^2
        };

        auto emitTriFlat = [&](const FVector3& p0, const FVector3& p1, const FVector3& p2)
        {
            if constexpr (bTimed)
                EmitTriTimedInternal(p0, p1, p2, color, style, seconds);
            else
                EmitTriInternal(m_ImmediateTris, p0, p1, p2, color, style);
        };

        auto emitTriN = [&](const FVector3& p0, const FVector3& p1, const FVector3& p2,
                            const FVector3& n0, const FVector3& n1, const FVector3& n2)
        {
            if constexpr (bTimed)
                EmitTriTimedInternalN(p0, p1, p2, n0, n1, n2, color, style, seconds);
            else
                EmitTriInternalN(m_ImmediateTris, p0, p1, p2, n0, n1, n2, color, style);
        };

        auto emitT = [&](const FVector3& p0, const FVector3& p1, const FVector3& p2)
        {
            if (!WantSmoothLitSolid(style))
            {
                emitTriFlat(p0, p1, p2);
                return;
            }

            const float tAvg =
                ((p0 - a).Dot(axis) + (p1 - a).Dot(axis) + (p2 - a).Dot(axis)) * (1.0f / 3.0f);
            const bool inHead = (tAvg >= headStartT);

            // Flat caps
            const FVector3 fn = faceNormal(p0, p1, p2);
            const float capness = std::fabs(fn.Dot(axis));
            const bool isCap = (capness > 0.92f);

            if (isCap)
            {
                const float sign = (fn.Dot(axis) >= 0.0f) ? 1.0f : -1.0f;
                const FVector3 nCap = axis * sign;
                emitTriN(p0, p1, p2, nCap, nCap, nCap);
                return;
            }

            if (inHead)
            {
                auto headN = [&](const FVector3& p) -> FVector3
                {
                    if (isSamePoint(p, b))
                        return apexN; // critical apex fix
                    return coneSideNormal(p);
                };

                emitTriN(p0, p1, p2, headN(p0), headN(p1), headN(p2));
            }
            else
            {
                emitTriN(p0, p1, p2, shaftNormal(p0), shaftNormal(p1), shaftNormal(p2));
            }
        };

        BuildArrowGeom(a, b, headLen, headRadius, headSegments, style.fill, emitL, emitT);
    }

    template <bool bTimed>
    void DrawCapsuleImpl(const FVector3& center, const FVector3& axisDir, float halfHeight, float radius,
                                    const FVector4& color, float seconds, int segments, const FDebugDrawStyle& style)
    {
        auto capsuleNormal = [&](const FVector3& p) -> FVector3
        {
            const FVector3 axis = NormalizeSafe(axisDir);
            const float t = (p - center).Dot(axis);
            const float tc = std::max(-halfHeight, std::min(halfHeight, t));
            const FVector3 closest = center + axis * tc;
            return NormalizeSafe(p - closest);
        };

        auto emitL = [&](const FVector3& p0, const FVector3& p1)
        {
            DrawLineImpl<bTimed>(p0, p1, color, seconds, style);
        };

        auto emitT = [&](const FVector3& p0, const FVector3& p1, const FVector3& p2)
        {
            if (WantSmoothLitSolid(style))
            {
                const FVector3 n0 = capsuleNormal(p0);
                const FVector3 n1 = capsuleNormal(p1);
                const FVector3 n2 = capsuleNormal(p2);

                if constexpr (bTimed)
                    EmitTriTimedInternalN(p0,p1,p2, n0,n1,n2, color, style, seconds);
                else
                    EmitTriInternalN(m_ImmediateTris, p0,p1,p2, n0,n1,n2, color, style);
            }
            else
            {
                if constexpr (bTimed)
                    EmitTriTimedInternal(p0,p1,p2, color, style, seconds);
                else
                    EmitTriInternal(m_ImmediateTris, p0,p1,p2, color, style);
            }
        };

        BuildCapsuleGeom(center, axisDir, halfHeight, radius, segments, style.fill, emitL, emitT);
    }


public:
    // Frame lifecycle
    void BeginFrame();
    void Tick(float dt);

    // Global toggles
    void SetEnabled(bool b) { m_bEnabled = b; }
    bool IsEnabled() const { return m_bEnabled; }

    void SetLayerEnabled(EDebugDrawLayer layer, bool b);
    bool IsLayerEnabled(EDebugDrawLayer layer) const;

    static FScreenPt ProjectToScreen(const FMatrix4& VP,
                                 const FVector3& world,
                                 int vx, int vy, int vw, int vh);

    /**
     * @brief mouseX_px/mouseY_px are absolute pixels in the same coordinate space as view.viewportX/Y/W/H.
     * radiusPx is a tolerance for clicking (like 6-10 px).
     */
    bool MouseHitTest(const FRenderView& view, const FMatrix4& viewMat, const FMatrix4& projMat,
                      float mouseX_px, float mouseY_px,
                      float radiusPx,
                      FDebugHit& outHit,
                      bool bRequireHitId = true) const;


    // Immediate primitives
    void DrawTri(const FVector3& a, const FVector3& b, const FVector3& c,
                 const FVector4& color, const FDebugDrawStyle& style = {});

    void DrawQuad(const FVector3& p0, const FVector3& p1, const FVector3& p2, const FVector3& p3,
                  const FVector4& color, const FDebugDrawStyle& style = {});

    void DrawLine(const FVector3& a, const FVector3& b, const FVector4& color,
                  const FDebugDrawStyle& style = {});
    void DrawRay(const FVector3& origin, const FVector3& dir, float len, const FVector4& color,
                 const FDebugDrawStyle& style = {});
    void DrawAxisTriad(const FTransform& t, float scale, const FDebugDrawStyle& style = {});

    void DrawCircle(const FVector3& center, const FVector3& normal, float radius, const FVector4& color,
                    int segments = 32, const FDebugDrawStyle& style = {});
    void DrawSphere(const FVector3& center, float radius, const FVector4& color,
                    int segments = 24, const FDebugDrawStyle& style = {});
    void DrawBox(const FVector3& center, const FVector3& halfExtents, const FVector4& color,
                 const FDebugDrawStyle& style = {});
    void DrawCylinder(const FVector3& baseCenter, const FVector3& axisDir, float height, float radius,
                      const FVector4& color, int segments = 24, const FDebugDrawStyle& style = {});
    void DrawCone(const FVector3& apex, const FVector3& dir, float height, float radius,
                  const FVector4& color, int segments = 24, const FDebugDrawStyle& style = {});
    void DrawArrow(const FVector3& a, const FVector3& b, const FVector4& color,
                   float headLen = 0.25f, float headRadius = 0.12f, int headSegments = 16,
                   const FDebugDrawStyle& style = {});
    void DrawCapsule(const FVector3& center, const FVector3& axisDir, float halfHeight, float radius,
                     const FVector4& color, int segments = 16, const FDebugDrawStyle& style = {});

    // Timed primitives
    void DrawLineTimed(const FVector3& a, const FVector3& b, const FVector4& color,
                       float seconds, const FDebugDrawStyle& style = {});
    void DrawRayTimed(const FVector3& origin, const FVector3& dir, float len, const FVector4& color,
                      float seconds, const FDebugDrawStyle& style = {});
    void DrawAxisTriadTimed(const FTransform& t, float scale, float seconds, const FDebugDrawStyle& style = {});

    void DrawCircleTimed(const FVector3& center, const FVector3& normal, float radius, const FVector4& color,
                         float seconds, int segments = 32, const FDebugDrawStyle& style = {});
    void DrawSphereTimed(const FVector3& center, float radius, const FVector4& color,
                         float seconds, int segments = 24, const FDebugDrawStyle& style = {});
    void DrawBoxTimed(const FVector3& center, const FVector3& halfExtents, const FVector4& color,
                      float seconds, const FDebugDrawStyle& style = {});
    void DrawCylinderTimed(const FVector3& baseCenter, const FVector3& axisDir, float height, float radius,
                           const FVector4& color, float seconds, int segments = 24, const FDebugDrawStyle& style = {});
    void DrawConeTimed(const FVector3& apex, const FVector3& dir, float height, float radius,
                       const FVector4& color, float seconds, int segments = 24, const FDebugDrawStyle& style = {});
    void DrawArrowTimed(const FVector3& a, const FVector3& b, const FVector4& color,
                        float seconds, float headLen = 0.25f, float headRadius = 0.12f, int headSegments = 16,
                        const FDebugDrawStyle& style = {});
    void DrawCapsuleTimed(const FVector3& center, const FVector3& axisDir, float halfHeight, float radius,
                          const FVector4& color, float seconds, int segments = 16, const FDebugDrawStyle& style = {});

    // Render hook
    void RenderForView(IRenderDevice& renderer, const FRenderView& view);
};
