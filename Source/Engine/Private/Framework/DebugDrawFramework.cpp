//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Framework/DebugDrawFramework.h"
#include "Rendering/IRenderDevice.h"
#include "Rendering/FRenderView.h"

#include <cstddef>

// ----------------------------
// Helpers
// ----------------------------

FVector3 DebugDraw::NormalizeSafe(const FVector3& v)
{
    const float len = v.Length();
    if (len <= 1e-6f) return FVector3(0,0,0);
    return v / len;
}

// ----------------------------
// Frame lifecycle
// ----------------------------

void DebugDraw::BeginFrame()
{
    m_Immediate.clear();
    m_ImmediateTris.clear();
}

void DebugDraw::Tick(float dt)
{
    for (size_t i = 0; i < m_Timed.size(); )
    {
        m_Timed[i].timeRemaining -= dt;
        if (m_Timed[i].timeRemaining <= 0.0f)
        {
            m_Timed[i] = m_Timed.back();
            m_Timed.pop_back();
            continue;
        }
        ++i;
    }

    for (size_t i = 0; i < m_TimedTris.size(); )
    {
        m_TimedTris[i].timeRemaining -= dt;
        if (m_TimedTris[i].timeRemaining <= 0.0f)
        {
            m_TimedTris[i] = m_TimedTris.back();
            m_TimedTris.pop_back();
            continue;
        }
        ++i;
    }
}

// ----------------------------
// Layer control
// ----------------------------

void DebugDraw::SetLayerEnabled(EDebugDrawLayer layer, bool b)
{
    const uint32_t bit = DebugLayerBit(layer);
    if (b) m_LayerMask |= bit;
    else   m_LayerMask &= ~bit;
}

bool DebugDraw::IsLayerEnabled(EDebugDrawLayer layer) const
{
    return (m_LayerMask & DebugLayerBit(layer)) != 0;
}

bool DebugDraw::MouseHitTest(const FRenderView& view, const FMatrix4& viewMat, const FMatrix4& projMat,
                             float mouseX_px, float mouseY_px,
                             float radiusPx,
                             FDebugHit& outHit,
                             bool bRequireHitId) const
{
    outHit = FDebugHit{};
    if (!m_bEnabled) return false;

    const FMatrix4 VP = projMat * viewMat;
    const FMatrix4 invVP = VP.Inverse();

    bool found = false;

    auto PassView = [&](const FDebugDrawStyle& s)
    {
        return (s.viewKey == 0) || (s.viewKey == view.viewIndex);
    };


    auto considerLine = [&](const FDebugLine& ln, uint32_t idx)
    {
        if (!PassLayer(ln.style.layer)) return;
        if (!PassView(ln.style)) return;
        if (bRequireHitId && ln.style.hitId == 0) return;

        const FScreenPt A = ProjectToScreen(VP, ln.a, view.viewportX, view.viewportY, view.viewportW, view.viewportH);
        const FScreenPt B = ProjectToScreen(VP, ln.b, view.viewportX, view.viewportY, view.viewportW, view.viewportH);
        if (!A.valid || !B.valid) return;

        const float tol = radiusPx + std::max(0.0f, ln.style.thicknessPx * 0.5f);
        const float d   = DistPointToSegment2D(mouseX_px, mouseY_px, A.x, A.y, B.x, B.y);
        if (d > tol) return;

        const float candDepth = std::min(A.depth01, B.depth01);

        bool take = false;
        if (!found) take = true;
        else
        {
            if (ln.style.depth == EDebugDepthMode::DepthTest)
                take = (candDepth < outHit.depth01);
            else
                take = (d < outHit.distPx);
        }

        if (!take) return;

        found = true;
        outHit.type    = FDebugHit::EType::Line;
        outHit.hitId   = ln.style.hitId;
        outHit.index   = idx;
        outHit.depth01 = candDepth;
        outHit.distPx  = d;
    };

    auto considerTri = [&](const FDebugTri& tr, uint32_t idx)
    {
        if (!PassLayer(tr.style.layer)) return;
        if (!PassView(tr.style)) return;
        if (bRequireHitId && tr.style.hitId == 0) return;

        // We usually only want solid tris pickable (gizmo planes, solid handles)
        if (tr.style.fill != EDebugFillMode::Solid)
            return;

        FVector3 ro, rd;
        if (!BuildRayFromMouse(view, invVP, mouseX_px, mouseY_px, ro, rd))
            return;

        float t,u,v;
        if (!RayTriIntersectMT(ro, rd, tr.a, tr.b, tr.c, t, u, v))
            return;

        const FVector3 hitPos = ro + rd * t;

        // Convert to a depth01 so DepthTest ordering matches screen projection
        const FScreenPt H = ProjectToScreen(VP, hitPos, view.viewportX, view.viewportY, view.viewportW, view.viewportH);
        const float candDepth = H.valid ? H.depth01 : 1.0f;

        bool take = false;
        if (!found) take = true;
        else
        {
            if (tr.style.depth == EDebugDepthMode::DepthTest)
                take = (candDepth < outHit.depth01);
            else
                take = (t < outHit.tRay); // overlay: nearest along the ray is usually what we want
        }

        if (!take) return;

        found = true;
        outHit.type     = FDebugHit::EType::Tri;
        outHit.hitId    = tr.style.hitId;
        outHit.index    = idx;
        outHit.depth01  = candDepth;
        outHit.tRay     = t;
        outHit.worldPos = hitPos;
    };

    // 1) immediate lines
    for (uint32_t i = 0; i < (uint32_t)m_Immediate.size(); ++i)
        considerLine(m_Immediate[i], i);

    // 2) timed lines
    for (uint32_t i = 0; i < (uint32_t)m_Timed.size(); ++i)
        considerLine(m_Timed[i].line, (uint32_t)m_Immediate.size() + i);

    // 3) immediate tris
    for (uint32_t i = 0; i < (uint32_t)m_ImmediateTris.size(); ++i)
        considerTri(m_ImmediateTris[i], i);

    // 4) timed tris
    for (uint32_t i = 0; i < (uint32_t)m_TimedTris.size(); ++i)
        considerTri(m_TimedTris[i].tri, (uint32_t)m_ImmediateTris.size() + i);

    return found;
}

void DebugDraw::DrawTri(const FVector3& a, const FVector3& b, const FVector3& c,
                        const FVector4& color, const FDebugDrawStyle& style)
{
    EmitTriInternal(m_ImmediateTris, a, b, c, color, style);
}

void DebugDraw::DrawQuad(const FVector3& p0, const FVector3& p1, const FVector3& p2, const FVector3& p3,
                         const FVector4& color, const FDebugDrawStyle& style)
{
    // two triangles
    EmitTriInternal(m_ImmediateTris, p0, p1, p2, color, style);
    EmitTriInternal(m_ImmediateTris, p0, p2, p3, color, style);
}

// ----------------------------
// Emit
// ----------------------------

void DebugDraw::EmitLineInternal(std::vector<FDebugLine>& dst,
                                 const FVector3& a, const FVector3& b,
                                 const FVector4& c, const FDebugDrawStyle& s)
{
    if (!m_bEnabled) return;
    if (!PassLayer(s.layer)) return;

    FDebugLine ln;
    ln.a = a;
    ln.b = b;
    ln.color = c;
    ln.style = s;
    dst.push_back(ln);
}

void DebugDraw::EmitTriInternal(std::vector<FDebugTri>& dst,
                                const FVector3& a, const FVector3& b, const FVector3& c,
                                const FVector4& col, const FDebugDrawStyle& s)
{
    if (!m_bEnabled) return;
    if (!PassLayer(s.layer)) return;

    FDebugTri t;
    t.a = a; t.b = b; t.c = c;
    t.color = col;
    t.style = s;
    dst.push_back(t);
}

void DebugDraw::EmitLineTimedInternal(const FVector3& a, const FVector3& b,
                                      const FVector4& c, const FDebugDrawStyle& s,
                                      float seconds)
{
    if (!m_bEnabled) return;
    if (!PassLayer(s.layer)) return;

    FTimedLine tl;
    tl.line.a = a;
    tl.line.b = b;
    tl.line.color = c;
    tl.line.style = s;
    tl.timeRemaining = seconds;
    m_Timed.push_back(tl);
}

void DebugDraw::EmitTriTimedInternal(const FVector3& a, const FVector3& b, const FVector3& c,
                                     const FVector4& col, const FDebugDrawStyle& s,
                                     float seconds)
{
    if (!m_bEnabled) return;
    if (!PassLayer(s.layer)) return;

    FTimedTri tt;
    tt.tri.a = a; tt.tri.b = b; tt.tri.c = c;
    tt.tri.color = col;
    tt.tri.style = s;
    tt.timeRemaining = seconds;
    m_TimedTris.push_back(tt);
}

FScreenPt DebugDraw::ProjectToScreen(const FMatrix4& VP,
                                     const FVector3& world,
                                     int vx, int vy, int vw, int vh)
{
    FScreenPt out{};

    const FVector4 clip = VP * FVector4(world.x, world.y, world.z, 1.0f);
    if (clip.w <= 1e-6f) return out; // behind / invalid

    const float ndcX = clip.x / clip.w;
    const float ndcY = clip.y / clip.w;
    const float ndcZ = clip.z / clip.w; // OpenGL-style: -1..1

    // Optionally reject if far outside; for gizmos you may allow some slack.
    if (ndcX < -1.5f || ndcX > 1.5f || ndcY < -1.5f || ndcY > 1.5f)
        return out;

    // NDC -> viewport pixels (origin top-left)
    const float sx = float(vx) + (ndcX * 0.5f + 0.5f) * float(vw);
    const float sy = float(vy) + (1.0f - (ndcY * 0.5f + 0.5f)) * float(vh);

    out.x = sx;
    out.y = sy;
    out.depth01 = ndcZ * 0.5f + 0.5f;
    out.valid = true;
    return out;
}

float DebugDraw::DistPointToSegment2D(float px, float py,
                                      float ax, float ay,
                                      float bx, float by)
{
    const float abx = bx - ax;
    const float aby = by - ay;
    const float apx = px - ax;
    const float apy = py - ay;

    const float abLen2 = abx*abx + aby*aby;
    if (abLen2 <= 1e-12f)
    {
        const float dx = px - ax;
        const float dy = py - ay;
        return std::sqrt(dx*dx + dy*dy);
    }

    float t = (apx*abx + apy*aby) / abLen2;
    t = std::max(0.0f, std::min(1.0f, t));

    const float cx = ax + abx * t;
    const float cy = ay + aby * t;

    const float dx = px - cx;
    const float dy = py - cy;
    return std::sqrt(dx*dx + dy*dy);
}

bool DebugDraw::BuildRayFromMouse(const FRenderView& view,
                                  const FMatrix4& invVP,
                                  float mouseX_px,
                                  float mouseY_px,
                                  FVector3& outOrigin,
                                  FVector3& outDir)
{
    // Require inside viewport
    if (mouseX_px < view.viewportX || mouseX_px > (view.viewportX + view.viewportW) ||
        mouseY_px < view.viewportY || mouseY_px > (view.viewportY + view.viewportH))
        return false;

    const float u = (mouseX_px - float(view.viewportX)) / float(view.viewportW);
    const float v = (mouseY_px - float(view.viewportY)) / float(view.viewportH);

    // NDC (-1..1), y flipped because screen is top-left origin
    const float ndcX = u * 2.0f - 1.0f;
    const float ndcY = 1.0f - v * 2.0f;

    const FVector4 pNearClip(ndcX, ndcY, -1.0f, 1.0f);
    const FVector4 pFarClip (ndcX, ndcY,  1.0f, 1.0f);

    FVector4 pNearW = invVP * pNearClip;
    FVector4 pFarW  = invVP * pFarClip;

    if (std::fabs(pNearW.w) <= 1e-6f || std::fabs(pFarW.w) <= 1e-6f)
        return false;

    pNearW = pNearW / pNearW.w;
    pFarW  = pFarW  / pFarW.w;

    outOrigin = FVector3(pNearW.x, pNearW.y, pNearW.z);
    outDir    = NormalizeSafe(FVector3(pFarW.x - pNearW.x, pFarW.y - pNearW.y, pFarW.z - pNearW.z));
    return outDir.Length() > 0.0f;
}

bool DebugDraw::RayTriIntersectMT(const FVector3& ro, const FVector3& rd,
                                  const FVector3& a, const FVector3& b, const FVector3& c,
                                  float& outT, float& outU, float& outV)
{
    const FVector3 e1 = b - a;
    const FVector3 e2 = c - a;

    const FVector3 p  = rd.Cross(e2);
    const float det   = e1.Dot(p);

    if (std::fabs(det) < 1e-8f) return false;
    const float invDet = 1.0f / det;

    const FVector3 s = ro - a;
    const float u = s.Dot(p) * invDet;
    if (u < 0.0f || u > 1.0f) return false;

    const FVector3 q = s.Cross(e1);
    const float v = rd.Dot(q) * invDet;
    if (v < 0.0f || (u + v) > 1.0f) return false;

    const float t = e2.Dot(q) * invDet;
    if (t < 0.0f) return false;

    outT = t; outU = u; outV = v;
    return true;
}

// ----------------------------
// Immediate: Lines
// ----------------------------

void DebugDraw::DrawLine(const FVector3& a, const FVector3& b, const FVector4& color,
                         const FDebugDrawStyle& style)
{
    EmitLineInternal(m_Immediate, a, b, color, style);
}

void DebugDraw::DrawRay(const FVector3& origin, const FVector3& dir, float len, const FVector4& color,
                        const FDebugDrawStyle& style)
{
    DrawLine(origin, origin + dir * len, color, style);
}

void DebugDraw::DrawAxisTriad(const FTransform& t, float scale, const FDebugDrawStyle& style)
{
    const FVector3 o = t.GetPosition();
    const FQuat q    = t.GetRotation();

    // LH, +X forward, +Y right, +Z up
    const FVector3 xAxis = q.RotateVector(FVector3(1, 0, 0));
    const FVector3 yAxis = q.RotateVector(FVector3(0, 1, 0));
    const FVector3 zAxis = q.RotateVector(FVector3(0, 0, 1));

    DrawLine(o, o + xAxis * scale, FVector4(1, 0, 0, 1), style); // X red
    DrawLine(o, o + yAxis * scale, FVector4(0, 1, 0, 1), style); // Y green
    DrawLine(o, o + zAxis * scale, FVector4(0, 0, 1, 1), style); // Z blue
}

// ----------------------------
// Immediate: Surfaces / Shapes
// ----------------------------

void DebugDraw::DrawCircle(const FVector3& center, const FVector3& normal, float radius, const FVector4& color,
                           int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineInternal(m_Immediate, a, b, color, style);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriInternal(m_ImmediateTris, a, b, c, color, style);
    };

    BuildCircleGeom(center, normal, radius, segments, style.fill, emitL, emitT);
}

void DebugDraw::DrawSphere(const FVector3& center, float radius, const FVector4& color,
                           int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineInternal(m_Immediate, a, b, color, style);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriInternal(m_ImmediateTris, a, b, c, color, style);
    };

    BuildSphereGeom(center, radius, segments, style.fill, emitL, emitT);
}

void DebugDraw::DrawBox(const FVector3& center, const FVector3& halfExtents, const FVector4& color,
                        const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineInternal(m_Immediate, a, b, color, style);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriInternal(m_ImmediateTris, a, b, c, color, style);
    };

    BuildBoxGeom(center, halfExtents, style.fill, emitL, emitT);
}

void DebugDraw::DrawCylinder(const FVector3& baseCenter, const FVector3& axisDir, float height, float radius,
                             const FVector4& color, int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineInternal(m_Immediate, a, b, color, style);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriInternal(m_ImmediateTris, a, b, c, color, style);
    };

    BuildCylinderGeom(baseCenter, axisDir, height, radius, segments, style.fill, emitL, emitT);
}

void DebugDraw::DrawCone(const FVector3& apex, const FVector3& dir, float height, float radius,
                         const FVector4& color, int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineInternal(m_Immediate, a, b, color, style);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriInternal(m_ImmediateTris, a, b, c, color, style);
    };

    BuildConeGeom(apex, dir, height, radius, segments, style.fill, emitL, emitT);
}

void DebugDraw::DrawArrow(const FVector3& a, const FVector3& b, const FVector4& color,
                          float headLen, float headRadius, int headSegments,
                          const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& p0, const FVector3& p1)
    {
        EmitLineInternal(m_Immediate, p0, p1, color, style);
    };
    auto emitT = [&](const FVector3& p0, const FVector3& p1, const FVector3& p2)
    {
        EmitTriInternal(m_ImmediateTris, p0, p1, p2, color, style);
    };

    BuildArrowGeom(a, b, headLen, headRadius, headSegments, style.fill, emitL, emitT);
}

void DebugDraw::DrawCapsule(const FVector3& center, const FVector3& axisDir, float halfHeight, float radius,
                            const FVector4& color, int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& p0, const FVector3& p1)
    {
        EmitLineInternal(m_Immediate, p0, p1, color, style);
    };
    auto emitT = [&](const FVector3& p0, const FVector3& p1, const FVector3& p2)
    {
        EmitTriInternal(m_ImmediateTris, p0, p1, p2, color, style);
    };

    BuildCapsuleGeom(center, axisDir, halfHeight, radius, segments, style.fill, emitL, emitT);
}

// ----------------------------
// Timed
// ----------------------------

void DebugDraw::DrawLineTimed(const FVector3& a, const FVector3& b, const FVector4& color,
                              float seconds, const FDebugDrawStyle& style)
{
    EmitLineTimedInternal(a, b, color, style, seconds);
}

void DebugDraw::DrawRayTimed(const FVector3& origin, const FVector3& dir, float len, const FVector4& color,
                             float seconds, const FDebugDrawStyle& style)
{
    DrawLineTimed(origin, origin + dir * len, color, seconds, style);
}

void DebugDraw::DrawAxisTriadTimed(const FTransform& t, float scale, float seconds, const FDebugDrawStyle& style)
{
    const FVector3 o = t.GetPosition();
    const FQuat q    = t.GetRotation();

    const FVector3 xAxis = q.RotateVector(FVector3(1, 0, 0));
    const FVector3 yAxis = q.RotateVector(FVector3(0, 1, 0));
    const FVector3 zAxis = q.RotateVector(FVector3(0, 0, 1));

    DrawLineTimed(o, o + xAxis * scale, FVector4(1, 0, 0, 1), seconds, style);
    DrawLineTimed(o, o + yAxis * scale, FVector4(0, 1, 0, 1), seconds, style);
    DrawLineTimed(o, o + zAxis * scale, FVector4(0, 0, 1, 1), seconds, style);
}

void DebugDraw::DrawCircleTimed(const FVector3& center, const FVector3& normal, float radius, const FVector4& color,
                                float seconds, int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineTimedInternal(a, b, color, style, seconds);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriTimedInternal(a, b, c, color, style, seconds);
    };

    BuildCircleGeom(center, normal, radius, segments, style.fill, emitL, emitT);
}

void DebugDraw::DrawSphereTimed(const FVector3& center, float radius, const FVector4& color,
                                float seconds, int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineTimedInternal(a, b, color, style, seconds);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriTimedInternal(a, b, c, color, style, seconds);
    };

    BuildSphereGeom(center, radius, segments, style.fill, emitL, emitT);
}

void DebugDraw::DrawBoxTimed(const FVector3& center, const FVector3& halfExtents, const FVector4& color,
                             float seconds, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineTimedInternal(a, b, color, style, seconds);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriTimedInternal(a, b, c, color, style, seconds);
    };

    BuildBoxGeom(center, halfExtents, style.fill, emitL, emitT);
}

void DebugDraw::DrawCylinderTimed(const FVector3& baseCenter, const FVector3& axisDir, float height, float radius,
                                  const FVector4& color, float seconds, int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineTimedInternal(a, b, color, style, seconds);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriTimedInternal(a, b, c, color, style, seconds);
    };

    BuildCylinderGeom(baseCenter, axisDir, height, radius, segments, style.fill, emitL, emitT);
}

void DebugDraw::DrawConeTimed(const FVector3& apex, const FVector3& dir, float height, float radius,
                              const FVector4& color, float seconds, int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& a, const FVector3& b)
    {
        EmitLineTimedInternal(a, b, color, style, seconds);
    };
    auto emitT = [&](const FVector3& a, const FVector3& b, const FVector3& c)
    {
        EmitTriTimedInternal(a, b, c, color, style, seconds);
    };

    BuildConeGeom(apex, dir, height, radius, segments, style.fill, emitL, emitT);
}

void DebugDraw::DrawArrowTimed(const FVector3& a, const FVector3& b, const FVector4& color,
                               float seconds, float headLen, float headRadius, int headSegments,
                               const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& p0, const FVector3& p1)
    {
        EmitLineTimedInternal(p0, p1, color, style, seconds);
    };
    auto emitT = [&](const FVector3& p0, const FVector3& p1, const FVector3& p2)
    {
        EmitTriTimedInternal(p0, p1, p2, color, style, seconds);
    };

    BuildArrowGeom(a, b, headLen, headRadius, headSegments, style.fill, emitL, emitT);
}

void DebugDraw::DrawCapsuleTimed(const FVector3& center, const FVector3& axisDir, float halfHeight, float radius,
                                 const FVector4& color, float seconds, int segments, const FDebugDrawStyle& style)
{
    auto emitL = [&](const FVector3& p0, const FVector3& p1)
    {
        EmitLineTimedInternal(p0, p1, color, style, seconds);
    };
    auto emitT = [&](const FVector3& p0, const FVector3& p1, const FVector3& p2)
    {
        EmitTriTimedInternal(p0, p1, p2, color, style, seconds);
    };

    BuildCapsuleGeom(center, axisDir, halfHeight, radius, segments, style.fill, emitL, emitT);
}

// ----------------------------
// Render
// ----------------------------

void DebugDraw::RenderForView(IRenderDevice& renderer, const FRenderView& view)
{
    if (!m_bEnabled) return;

    auto PassView = [&](const FDebugDrawStyle& s)
    {
        return (s.viewKey == 0) || (s.viewKey == view.viewIndex);
    };

    std::vector<FDebugLine> lines;
    lines.reserve(m_Immediate.size() + m_Timed.size());

    std::vector<FDebugTri> tris;
    tris.reserve(m_ImmediateTris.size() + m_TimedTris.size());

    auto acceptLine = [&](const FDebugLine& ln)
    {
        if (!PassLayer(ln.style.layer)) return;
        if (!PassView(ln.style)) return;
        lines.push_back(ln);
    };

    auto acceptTri = [&](const FDebugTri& tr)
    {
        if (!PassLayer(tr.style.layer)) return;
        if (!PassView(tr.style)) return;
        tris.push_back(tr);
    };

    for (const auto& ln : m_Immediate) acceptLine(ln);
    for (const auto& tl : m_Timed)     acceptLine(tl.line);

    for (const auto& tr : m_ImmediateTris) acceptTri(tr);
    for (const auto& tt : m_TimedTris)     acceptTri(tt.tri);

    if (!lines.empty())
        renderer.SubmitDebugLines(view, lines.data(), (uint32_t)lines.size());

    if (!tris.empty())
        renderer.SubmitDebugTriangles(view, tris.data(), (uint32_t)tris.size());
}
