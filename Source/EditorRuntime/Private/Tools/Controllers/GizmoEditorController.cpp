//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Tools/Controllers/GizmoEditorController.h"

#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Rendering/FRenderView.h"
#include "Core/Math/FIntersection.h"
#include "Core/Math/FViewportMath.h"

static bool IsTranslateAxis(GizmoEditorTool::EHandle h)
{
    return h == GizmoEditorTool::EHandle::T_X ||
           h == GizmoEditorTool::EHandle::T_Y ||
           h == GizmoEditorTool::EHandle::T_Z;
}

static bool IsTranslateCenter(GizmoEditorTool::EHandle h)
{
    return h == GizmoEditorTool::EHandle::T_Center;
}

static bool IsTranslatePlane(GizmoEditorTool::EHandle h)
{
    return h == GizmoEditorTool::EHandle::T_XY ||
           h == GizmoEditorTool::EHandle::T_XZ ||
           h == GizmoEditorTool::EHandle::T_YZ;
}

static bool IsRotateAxis(GizmoEditorTool::EHandle h)
{
    return h == GizmoEditorTool::EHandle::R_X ||
           h == GizmoEditorTool::EHandle::R_Y ||
           h == GizmoEditorTool::EHandle::R_Z;
}

static bool IsScaleAxis(GizmoEditorTool::EHandle h)
{
    return h == GizmoEditorTool::EHandle::S_X ||
           h == GizmoEditorTool::EHandle::S_Y ||
           h == GizmoEditorTool::EHandle::S_Z;
}

static bool IsScaleUniform(GizmoEditorTool::EHandle h)
{
    return h == GizmoEditorTool::EHandle::S_Uniform;
}

static FVector3 HandleAxisWS(GizmoEditorTool::EHandle h,
                             const FVector3& X, const FVector3& Y, const FVector3& Z)
{
    switch (h)
    {
        case GizmoEditorTool::EHandle::T_X:
        case GizmoEditorTool::EHandle::R_X:
        case GizmoEditorTool::EHandle::S_X: return X;

        case GizmoEditorTool::EHandle::T_Y:
        case GizmoEditorTool::EHandle::R_Y:
        case GizmoEditorTool::EHandle::S_Y: return Y;

        case GizmoEditorTool::EHandle::T_Z:
        case GizmoEditorTool::EHandle::R_Z:
        case GizmoEditorTool::EHandle::S_Z: return Z;
        default: return FVector3(1,0,0);
    }
}

static float SignedAngleAroundAxis(const FVector3& aUnit,
                                   const FVector3& bUnit,
                                   const FVector3& axisUnit)
{
    const float c = FMath::Clamp(aUnit.Dot(bUnit), -1.0f, 1.0f);
    const float angle = std::acos(c);

    const float s = axisUnit.Dot(aUnit.Cross(bUnit)); // sign from right-hand rule in math space
    return (s < 0.0f) ? -angle : angle;
}

static float ComputeGizmoScaleMulFromProjection(const FMatrix4& projMat, int viewportH)
{
    const float projY = projMat.GetMat4()[1][1];

    if (projY <= 0.00001f)
        return 1.0f;

    // tan(fovY/2) = 1/projY
    const float tanHalfFovY = 1.0f / projY;

    // tuned reference: 60° vertical
    constexpr float kRefTanHalfFovY = 0.57735026919f; // tan(30°)

    float mul = tanHalfFovY / kRefTanHalfFovY;

    // Stabilize across viewport pixel heights
    constexpr float kRefViewportH = 720.0f;
    {
        float hMul = kRefViewportH / float(viewportH);
        hMul = std::clamp(hMul, 0.2f, 1.8f); // tune clamp
        mul *= hMul;
    }
    return mul;
}

GizmoEditorTool::EHandle GizmoEditorController::HitTest(const DebugDraw& debugDraw,
                                                        const FRenderView& view,
                                                        const FMatrix4& viewMat,
                                                        const FMatrix4& projMat,
                                                        float mouseX_px,
                                                        float mouseY_px) const
{
    if (m_BaseHitID == 0)
        return GizmoEditorTool::EHandle::None;

    FDebugHit hit{};
    const bool bHit = debugDraw.MouseHitTest(view, viewMat, projMat,
                                      mouseX_px, mouseY_px,
                                      m_Cfg.hitRadiusPx,
                                      hit,
                                      true);

    if (!bHit || hit.hitId == 0)
        return GizmoEditorTool::EHandle::None;

    GizmoEditorTool gizmo;
    return gizmo.HitIdToHandle(m_BaseHitID, hit.hitId);
}

FRay GizmoEditorController::MakeMouseRayWS(const FRenderView &view, const FMatrix4 &viewMat, const FMatrix4 &projMat,
    float mouseX_px, float mouseY_px)
{
    return FViewportMath::BuildRayFromViewProj(
        viewMat, projMat,
        float(view.viewportW), float(view.viewportH),
        mouseX_px, mouseY_px
    );
}

void GizmoEditorController::BuildBasisWS(const FTransform &xf, GizmoEditorTool::ESpace space, FVector3 &outX,
                                         FVector3 &outY, FVector3 &outZ)
{
    GizmoEditorTool::BuildBasis(xf, space, outX, outY, outZ);
}

bool GizmoEditorController::RayPlaneThroughPoint(const FRay &ray, const FVector3 &P0, const FVector3 &N,
    FVector3 &outHit)
{
    float t = 0.0f;
    if (!FIntersection::RayPlane(ray, P0, N, t, &outHit))
        return false;
    return true;
}

bool GizmoEditorController::RayAxisClosestS(const FRay &ray, const FVector3 &P0, const FVector3 &A_unit, float &outS)
{
    const FVector3 D = ray.direction.Normalized(); // assume normalized
    const FVector3 w0 = ray.origin - P0;

    const float a = D.Dot(D); // ~1
    const float b = D.Dot(A_unit);
    const float c = A_unit.Dot(A_unit); // 1
    const float d = D.Dot(w0);
    const float e = A_unit.Dot(w0);

    const float denom = a*c - b*b;
    if (std::fabs(denom) < 1e-6f)
        return false; // near-parallel

    const float s = (a*e - b*d) / denom;
    outS = s;
    return true;
}

FVector3 GizmoEditorController::AxisDragPlaneNormal(const FVector3 &axisUnit, const FVector3 &camFwdUnit)
{
    // n = axis x (camFwd x axis)
    const FVector3 c = camFwdUnit.Cross(axisUnit);
    const float clen2 = c.Dot(c);
    if (clen2 < 1e-6f)
    {
        // camera looking nearly along axis; pick any perpendicular
        FVector3 any = (std::fabs(axisUnit.z) < 0.9f) ? FVector3(0,0,1) : FVector3(0,1,0);
        return axisUnit.Cross(any).Normalized();
    }
    return axisUnit.Cross(c).Normalized();
}

float GizmoEditorController::ComputeFadeMul(const FMatrix4 &viewMat, const FVector3 &gizmoPosWS)
{
    // view-space position
    const FVector4 pVS4 = viewMat * FVector4(gizmoPosWS.x, gizmoPosWS.y, gizmoPosWS.z, 1.0f);
    const float zVS = std::fabs(pVS4.x); // LH: X forward => depth is X in engine's convention (camera forward is +X)

    // If depth is too small, fade out
    // Tune these as "meters" in view space.
    constexpr float kFadeStart = 3.f;
    constexpr float kFadeEnd   = 0.32f;

    // smoothstep(End..Start)
    float t = (zVS - kFadeEnd) / (kFadeStart - kFadeEnd);
    t = std::clamp(t, 0.0f, 1.0f);
    // smoother step
    //t = t*t*t * (t*(t*6 - 15) + 10); // 6t^5 - 15t^4 + 10t^3

    return t; // 1 far, 0 very close
}

void GizmoEditorController::CancelCapture()
{
    m_bCapturing = false;
    m_Active = GizmoEditorTool::EHandle::None;
    EndDrag();
}

bool GizmoEditorController::BeginDrag(const FRay &rayWS, const FVector3 &camFwd, const FTransform &gizmoXf)
{
    m_Drag = {}; // reset
    m_Drag.bActive = true;
    m_Drag.mode    = m_Mode;
    m_Drag.space   = m_Space;
    m_Drag.handle  = m_Active;

    m_Drag.gizmoStartXf = gizmoXf;
    m_Drag.pivotWS = gizmoXf.GetPosition();

    BuildBasisWS(gizmoXf, m_Space, m_Drag.X, m_Drag.Y, m_Drag.Z);

    // Determine constraint setup based on handle type
    if (IsTranslateAxis(m_Active) || IsRotateAxis(m_Active) || IsScaleAxis(m_Active))
    {
        m_Drag.axisWS = HandleAxisWS(m_Active, m_Drag.X, m_Drag.Y, m_Drag.Z).Normalized();

        if (IsRotateAxis(m_Active))
            m_Drag.planeNormalWS = m_Drag.axisWS; // rotate in plane normal to axis
        else
            m_Drag.planeNormalWS = AxisDragPlaneNormal(m_Drag.axisWS, camFwd.Normalized());
    }
    else if (IsTranslatePlane(m_Active))
    {
        // Plane handle chooses plane normal from basis:
        // XY => normal Z, XZ => normal Y, YZ => normal X
        if (m_Active == GizmoEditorTool::EHandle::T_XY) m_Drag.planeNormalWS = m_Drag.Z;
        if (m_Active == GizmoEditorTool::EHandle::T_XZ) m_Drag.planeNormalWS = m_Drag.Y;
        if (m_Active == GizmoEditorTool::EHandle::T_YZ) m_Drag.planeNormalWS = m_Drag.X;
        m_Drag.planeNormalWS = m_Drag.planeNormalWS.Normalized();
    }
    else if (IsTranslateCenter(m_Active))
    {
        // Free move: plane through pivot facing camera
        m_Drag.planeNormalWS = camFwd.Normalized();
    }
    else if (IsScaleUniform(m_Active))
    {
        // Uniform scale: use camera-facing plane to measure radial distance from pivot
        m_Drag.planeNormalWS = camFwd.Normalized();
    }
    else
    {
        // Unknown / None
        m_Drag.bActive = false;
        return false;
    }

    // Start hit on the chosen plane (pivot + planeNormal)
    {
        FVector3 hit{};
        if (RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit))
        {
            m_Drag.bHasStartHit = true;
            m_Drag.startHitWS = hit;

            if (IsRotateAxis(m_Active))
            {
                FVector3 v = (hit - m_Drag.pivotWS);
                const float len = v.Length();
                m_Drag.prevVecWS = (len > 1e-6f) ? (v / len) : FVector3(1,0,0);
                m_Drag.angleAccumRad = 0.0f;
            }

            // Uniform scale start distance
            if (IsScaleUniform(m_Active)) // TODO: Make the uniform scale drag feel better (UX)
            {
                // direction in the camera-facing plane (pivot -> startHit), but planar
                FVector3 v0 = hit - m_Drag.pivotWS;
                v0 -= m_Drag.planeNormalWS * v0.Dot(m_Drag.planeNormalWS);

                const float len = v0.Length();
                if (len < 1e-4f)
                    return false; // clicked too close to pivot, can't define direction

                m_Drag.uniformDirWS = v0 / len;
                m_Drag.bHasUniformDir = true;
            }
        }
        // no plane hit is not fatal for axis translate because we can use closest-S;
        // but for rotate and plane translate it is required.
    }

    // Also capture axis param start if possible (stable for axis translate/scale)
    if (IsTranslateAxis(m_Active) || IsScaleAxis(m_Active))
    {
        float s0 = 0.0f;
        if (RayAxisClosestS(rayWS, m_Drag.pivotWS, m_Drag.axisWS, s0))
        {
            m_Drag.bHasAxisSStart = true;
            m_Drag.axisSStart = s0;
        }
        // fallback will use plane-hit projection if closestS is unstable
    }

    // Validate required start data
    if (IsTranslateCenter(m_Active) && !m_Drag.bHasStartHit) return false;
    if (IsScaleUniform(m_Active) && (!m_Drag.bHasStartHit || !m_Drag.bHasUniformDir)) return false;
    if (IsTranslatePlane(m_Active) && !m_Drag.bHasStartHit) return false;
    if (IsRotateAxis(m_Active)    && !m_Drag.bHasStartHit) return false;

    return true;
}

bool GizmoEditorController::UpdateDrag(const FRay &rayWS, const FVector3 &camFwd, FGizmoTransformDelta &outDelta)
{
    if (!m_Drag.bActive) return false;

    outDelta = {};
    outDelta.bHasDelta = false;
    outDelta.mode   = m_Drag.mode;
    outDelta.space  = m_Drag.space;
    outDelta.handle = m_Drag.handle;
    outDelta.pivotWS = m_Drag.pivotWS;

    // ----- TRANSLATE -----
    if (m_Drag.mode == GizmoEditorTool::EMode::Translate)
    {
        // Plane translate
        if (IsTranslatePlane(m_Drag.handle))
        {
            FVector3 hit{};
            if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit))
                return true; // no update

            FVector3 delta = hit - m_Drag.startHitWS;

            // ensure planar constraint even if numerical drift occurs
            delta -= m_Drag.planeNormalWS * delta.Dot(m_Drag.planeNormalWS);

            outDelta.deltaTranslationWS = delta;
            outDelta.bHasDelta = true;
            return true;
        }

        // Axis translate
        if (IsTranslateAxis(m_Drag.handle))
        {
            float s = 0.0f;
            if (m_Drag.bHasAxisSStart && RayAxisClosestS(rayWS, m_Drag.pivotWS, m_Drag.axisWS, s))
            {
                const float ds = s - m_Drag.axisSStart;
                outDelta.deltaTranslationWS = m_Drag.axisWS * ds;
                outDelta.bHasDelta = true;
                return true;
            }

            // fallback: camera-facing plane intersection then project onto axis
            FVector3 hit{};
            if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit) || !m_Drag.bHasStartHit)
                return true;

            const FVector3 raw = hit - m_Drag.startHitWS;
            const float ds = raw.Dot(m_Drag.axisWS);

            outDelta.deltaTranslationWS = m_Drag.axisWS * ds;
            outDelta.bHasDelta = true;
            return true;
        }

        // Center translate: free move on camera-facing plane
        if (IsTranslateCenter(m_Drag.handle))
        {
            FVector3 hit{};
            if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit) || !m_Drag.bHasStartHit)
                return true;

            outDelta.deltaTranslationWS = hit - m_Drag.startHitWS;
            outDelta.bHasDelta = true;
            return true;
        }

        return true;
    }

    // ----- ROTATE -----
    if (m_Drag.mode == GizmoEditorTool::EMode::Rotate && IsRotateAxis(m_Drag.handle))
    {
        FVector3 hit{};
        if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit))
            return true;

        FVector3 v = hit - m_Drag.pivotWS;
        const float len = v.Length();
        if (len < 1e-6f) return true;

        const FVector3 vCur = v / len;

        // incremental accumulation avoids big jumps near 180°
        const float dAng = SignedAngleAroundAxis(m_Drag.prevVecWS, vCur, m_Drag.axisWS);
        m_Drag.angleAccumRad += dAng;
        m_Drag.prevVecWS = vCur;

        outDelta.rotationAxisWS = m_Drag.axisWS;
        outDelta.rotationAngleRad = m_Drag.angleAccumRad;
        outDelta.bHasDelta = true;
        return true;
    }

    // ----- SCALE (UNIFORM) -----
    if (m_Drag.mode == GizmoEditorTool::EMode::Scale && IsScaleUniform(m_Drag.handle))
    {
        FVector3 hit{};
        if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit) ||
            !m_Drag.bHasStartHit || !m_Drag.bHasUniformDir)
            return true;

        // signed movement along the initial radial direction on the plane
        const FVector3 v = hit - m_Drag.startHitWS;
        const float ds = v.Dot(m_Drag.uniformDirWS);

        float u = 1.0f + ds * m_Cfg.scaleSensitivity;
        u = std::max(u, m_Cfg.minAxisScaleMul);

        outDelta.deltaScaleMul = FVector3(u,u,u);
        outDelta.bHasDelta = true;
        return true;
    }

    // ----- SCALE -----
    if (m_Drag.mode == GizmoEditorTool::EMode::Scale && IsScaleAxis(m_Drag.handle))
    {
        // Use camera-facing plane then project onto axis for stable feel
        FVector3 hit{};
        if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit) || !m_Drag.bHasStartHit)
            return true;

        const float ds = (hit - m_Drag.startHitWS).Dot(m_Drag.axisWS);

        float axisMul = 1.0f + ds * m_Cfg.scaleSensitivity;
        axisMul = std::max(axisMul, m_Cfg.minAxisScaleMul);

        // Convert into basis component scale (X/Y/Z correspond to m_Drag basis)
        FVector3 mul(1,1,1);
        // Which basis axis is this handle?
        if (m_Drag.handle == GizmoEditorTool::EHandle::S_X) mul.x = axisMul;
        if (m_Drag.handle == GizmoEditorTool::EHandle::S_Y) mul.y = axisMul;
        if (m_Drag.handle == GizmoEditorTool::EHandle::S_Z) mul.z = axisMul;

        outDelta.deltaScaleMul = mul;
        outDelta.bHasDelta = true;
        return true;
    }

    return true;
}

void GizmoEditorController::EndDrag()
{
    m_Drag = {};
}

GizmoEditorController::FResult GizmoEditorController::UpdateAndDraw(DebugDraw& debugDraw,
                                                                    const FRenderView& view,
                                                                    const FMatrix4& viewMat,
                                                                    const FMatrix4& projMat,
                                                                    const FVector3& camPos,
                                                                    const FVector3& camFwd,
                                                                    const FTransform& gizmoXf,
                                                                    bool bDraw,
                                                                    bool bAllowBeginCapture,
                                                                    const FViewportPanelInput& input)
{
    FResult result{};

    if (!bDraw)
    {
        CancelCapture();
        m_Hovered = GizmoEditorTool::EHandle::None;
        result.hovered = m_Hovered;
        result.active = m_Active;
        return result;
    }

    // A) Hover pass (only when not capturing)
    GizmoEditorTool::EHandle newHover = GizmoEditorTool::EHandle::None;

    if (!m_bCapturing && input.bFocused && input.bOverViewport)
    {
        GizmoEditorTool::FDrawParams pick{};
        pick.mode = m_Mode;
        pick.space = m_Space;
        pick.baseHitID = m_BaseHitID;
        pick.hoveredHandle = GizmoEditorTool::EHandle::None;
        pick.activeHandle  = GizmoEditorTool::EHandle::None;
        pick.bDrawSphereHint = true;
        pick.scaleMul = ComputeGizmoScaleMulFromProjection(projMat, view.viewportH);
        pick.alphaMul = ComputeFadeMul(viewMat, gizmoXf.GetPosition());

        m_GizmoTool.Draw(debugDraw, view.viewIndex, camPos, camFwd, gizmoXf, pick);
        newHover = HitTest(debugDraw, view, viewMat, projMat, input.mouseX, input.mouseY);
    }

    // Precompute ray (used for begin/update)
    const FRay rayWS = MakeMouseRayWS(view, viewMat, projMat, input.mouseX, input.mouseY);

    // B) Capture begin / update / end
    if (!m_bCapturing)
    {
        m_Hovered = newHover;

        if (bAllowBeginCapture && input.bOverViewport && input.bLeftClicked &&
            newHover != GizmoEditorTool::EHandle::None)
        {
            m_bCapturing = true;
            m_Active = newHover;

            // Begin drag snapshot
            const bool bDrag = BeginDrag(rayWS, camFwd, gizmoXf);
            if (!bDrag)
            {
                // Could not initialize stable drag (parallel ray/plane etc.)
                CancelCapture();
            }
            else
            {
                result.bBeganCapture = true;
                result.bWantsCapture = true;
                result.bConsumesClick = m_Cfg.bGizmoFirst;
            }
        }
    }
    else
    {
        // While capturing we suppress hover and always request capture routing.
        m_Hovered = GizmoEditorTool::EHandle::None;
        result.bWantsCapture = true;
        result.bConsumesClick = true; // while dragging, always block picking

        // Compute delta
        UpdateDrag(rayWS, camFwd, result.manipulation);

        // End capture on release
        if (input.bLeftReleased)
        {
            result.bEndedCapture = true;
            CancelCapture();
        }
    }

    // C) Final draw (highlighted state)
    GizmoEditorTool::FDrawParams finalP{};
    finalP.mode = m_Mode;
    finalP.space = m_Space;
    finalP.baseHitID = m_BaseHitID;
    finalP.hoveredHandle = m_Hovered;
    finalP.activeHandle  = m_Active;
    finalP.bDrawSphereHint = true;
    finalP.scaleMul = ComputeGizmoScaleMulFromProjection(projMat, view.viewportH);
    finalP.alphaMul = ComputeFadeMul(viewMat, gizmoXf.GetPosition());

    m_GizmoTool.Draw(debugDraw, view.viewIndex, camPos, camFwd, gizmoXf, finalP);

    result.hovered = m_Hovered;
    result.active  = m_Active;
    return result;
}