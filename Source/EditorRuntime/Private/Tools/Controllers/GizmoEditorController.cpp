//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Tools/Controllers/GizmoEditorController.h"

#include <algorithm>
#include <cmath>

#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Core/Math/FIntersection.h"
#include "Core/Math/FViewportMath.h"
#include "Rendering/FRenderView.h"

// ----------------------
// Local helpers
// ----------------------

namespace
{
    using EHandle = GizmoEditorTool::EHandle;

    inline bool IsTranslateAxis(EHandle h)   { return h == EHandle::T_X || h == EHandle::T_Y || h == EHandle::T_Z; }
    inline bool IsTranslatePlane(EHandle h)  { return h == EHandle::T_XY || h == EHandle::T_XZ || h == EHandle::T_YZ; }
    inline bool IsTranslateCenter(EHandle h) { return h == EHandle::T_Center; }

    inline bool IsRotateAxis(EHandle h)      { return h == EHandle::R_X || h == EHandle::R_Y || h == EHandle::R_Z; }
    inline bool IsRotateFree(EHandle h) { return h == EHandle::R_Free; }

    inline bool IsScaleAxis(EHandle h)       { return h == EHandle::S_X || h == EHandle::S_Y || h == EHandle::S_Z; }
    inline bool IsScalePlane(EHandle h)      { return h == EHandle::S_XY || h == EHandle::S_XZ || h == EHandle::S_YZ; }
    inline bool IsScaleUniform(EHandle h)    { return h == EHandle::S_Uniform; }

    inline FVector3 HandleAxisWS(EHandle h, const FVector3& X, const FVector3& Y, const FVector3& Z)
    {
        switch (h)
        {
            case EHandle::T_X:
            case EHandle::R_X:
            case EHandle::S_X: return X;

            case EHandle::T_Y:
            case EHandle::R_Y:
            case EHandle::S_Y: return Y;

            case EHandle::T_Z:
            case EHandle::R_Z:
            case EHandle::S_Z: return Z;

            default: return FVector3(1,0,0);
        }
    }

    inline float SignedAngleAroundAxis(const FVector3& aUnit, const FVector3& bUnit, const FVector3& axisUnit)
    {
        const float c = FMath::Clamp(aUnit.Dot(bUnit), -1.0f, 1.0f);
        const float angle = std::acos(c);
        const float s = axisUnit.Dot(aUnit.Cross(bUnit));
        return (s < 0.0f) ? -angle : angle;
    }

    inline float ComputeGizmoScaleMulFromProjection(const FMatrix4& projMat, int viewportH)
    {
        const float projY = projMat.GetMat4()[1][1];
        if (projY <= 0.00001f) return 1.0f;

        // tan(fovY/2) = 1/projY
        const float tanHalfFovY = 1.0f / projY;

        // tuned reference: 60° vertical (tan(30°))
        constexpr float kRefTanHalfFovY = 0.57735026919f;
        float mul = tanHalfFovY / kRefTanHalfFovY;

        // Stabilize across viewport pixel heights
        constexpr float kRefViewportH = 720.0f;
        float hMul = kRefViewportH / float(viewportH);
        hMul = std::clamp(hMul, 0.2f, 1.8f);
        mul *= hMul;

        return mul;
    }

    inline bool IsAnyPlaneHandle(EHandle h) { return IsTranslatePlane(h) || IsScalePlane(h); }
    inline bool IsAnyAxisHandle(EHandle h)  { return IsTranslateAxis(h) || IsRotateAxis(h) || IsScaleAxis(h); }
    inline bool IsRotateHandle(EHandle h) { return IsRotateAxis(h) || IsRotateFree(h); }
} // namespace

// ----------------------
// GizmoEditorController
// ----------------------

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
    const bool bHit = debugDraw.MouseHitTest(
        view, viewMat, projMat,
        mouseX_px, mouseY_px,
        m_Cfg.hitRadiusPx,
        hit,
        /*bEditorLayerOnly*/true
    );

    if (!bHit || hit.hitId == 0)
        return GizmoEditorTool::EHandle::None;

    return m_GizmoTool.HitIdToHandle(m_BaseHitID, hit.hitId);
}

FRay GizmoEditorController::MakeMouseRayWS(const FRenderView& view,
                                          const FMatrix4& viewMat,
                                          const FMatrix4& projMat,
                                          float mouseX_px,
                                          float mouseY_px)
{
    return FViewportMath::BuildRayFromViewProj(
        viewMat, projMat,
        float(view.viewportW), float(view.viewportH),
        mouseX_px, mouseY_px
    );
}

void GizmoEditorController::BuildBasisWS(const FTransform& xf,
                                        GizmoEditorTool::ESpace space,
                                        FVector3& outX,
                                        FVector3& outY,
                                        FVector3& outZ)
{
    GizmoEditorTool::BuildBasis(xf, space, outX, outY, outZ);
}

bool GizmoEditorController::RayPlaneThroughPoint(const FRay& ray,
                                                 const FVector3& P0,
                                                 const FVector3& N,
                                                 FVector3& outHit)
{
    float t = 0.0f;
    return FIntersection::RayPlane(ray, P0, N, t, &outHit);
}

bool GizmoEditorController::RayAxisClosestS(const FRay& ray,
                                            const FVector3& P0,
                                            const FVector3& A_unit,
                                            float& outS)
{
    const FVector3 D = ray.direction.Normalized();
    const FVector3 w0 = ray.origin - P0;

    const float a = D.Dot(D);        // ~1
    const float b = D.Dot(A_unit);
    const float c = A_unit.Dot(A_unit); // 1
    const float d = D.Dot(w0);
    const float e = A_unit.Dot(w0);

    const float denom = a * c - b * b;
    if (std::fabs(denom) < 1e-6f)
        return false;

    outS = (a * e - b * d) / denom;
    return true;
}

FVector3 GizmoEditorController::AxisDragPlaneNormal(const FVector3& axisUnit, const FVector3& camFwdUnit)
{
    const FVector3 c = camFwdUnit.Cross(axisUnit);
    const float clen2 = c.Dot(c);
    if (clen2 < 1e-6f)
    {
        FVector3 any = (std::fabs(axisUnit.z) < 0.9f) ? FVector3(0,0,1) : FVector3(0,1,0);
        return axisUnit.Cross(any).Normalized();
    }
    return axisUnit.Cross(c).Normalized();
}

float GizmoEditorController::ComputeFadeMul(const FMatrix4& viewMat, const FVector3& gizmoPosWS)
{
    const FVector4 pVS4 = viewMat * FVector4(gizmoPosWS.x, gizmoPosWS.y, gizmoPosWS.z, 1.0f);

    // LH: X forward => depth is X in engine's convention
    const float zVS = std::fabs(pVS4.x);

    constexpr float kFadeStart = 3.f;
    constexpr float kFadeEnd   = 0.32f;

    float t = (zVS - kFadeEnd) / (kFadeStart - kFadeEnd);
    return std::clamp(t, 0.0f, 1.0f);
}

void GizmoEditorController::CancelCapture()
{
    m_bCapturing = false;
    m_Active = GizmoEditorTool::EHandle::None;
    EndDrag();
}

bool GizmoEditorController::BeginDrag(const FRay& rayWS, const FVector3& camFwd, const FTransform& gizmoXf)
{
    m_Drag = {};
    m_Drag.bActive = true;
    m_Drag.mode    = m_Mode;
    m_Drag.space   = m_Space;
    m_Drag.handle  = m_Active;

    m_Drag.gizmoStartXf = gizmoXf;
    m_Drag.pivotWS = gizmoXf.GetPosition();

    BuildBasisWS(gizmoXf, m_Space, m_Drag.X, m_Drag.Y, m_Drag.Z);

    const FVector3 camFwdUnit = camFwd.Normalized();

    // Setup constraints
    if (IsRotateFree(m_Active))
    {
        const FVector3 viewAxis = camFwdUnit;
        m_Drag.axisWS       = viewAxis;
        m_Drag.planeNormalWS = viewAxis; // drag plane perpendicular to view axis
    }
    else if (IsAnyAxisHandle(m_Active))
    {
        m_Drag.axisWS = HandleAxisWS(m_Active, m_Drag.X, m_Drag.Y, m_Drag.Z).Normalized();

        if (IsRotateAxis(m_Active))
            m_Drag.planeNormalWS = m_Drag.axisWS;
        else
            m_Drag.planeNormalWS = AxisDragPlaneNormal(m_Drag.axisWS, camFwdUnit);
    }
    else if (IsAnyPlaneHandle(m_Active))
    {
        // XY => normal Z, XZ => normal Y, YZ => normal X (works for both translate+scale plane handles)
        switch (m_Active)
        {
            case EHandle::T_XY:
            case EHandle::S_XY: m_Drag.planeNormalWS = m_Drag.Z; break;

            case EHandle::T_XZ:
            case EHandle::S_XZ: m_Drag.planeNormalWS = m_Drag.Y; break;

            case EHandle::T_YZ:
            case EHandle::S_YZ: m_Drag.planeNormalWS = m_Drag.X; break;

            default: break;
        }
        m_Drag.planeNormalWS = m_Drag.planeNormalWS.Normalized();
    }
    else if (IsTranslateCenter(m_Active) || IsScaleUniform(m_Active))
    {
        m_Drag.planeNormalWS = camFwdUnit;
    }
    else
    {
        m_Drag.bActive = false;
        return false;
    }

    // Start hit on the chosen plane
    {
        FVector3 hit{};
        if (RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit))
        {
            m_Drag.bHasStartHit = true;
            m_Drag.startHitWS = hit;

            if (IsRotateHandle(m_Active))
            {
                FVector3 v = (hit - m_Drag.pivotWS);
                const float len = v.Length();
                if (len < 1e-6f)
                    return false; // can't define a direction reliably

                m_Drag.prevVecWS = v / len;
                m_Drag.angleAccumRad = 0.0f;
                m_Drag.bSkipFirstRotateUpdate = true;
            }

            if (IsScaleUniform(m_Active))
            {
                FVector3 v0 = hit - m_Drag.pivotWS;
                v0 -= m_Drag.planeNormalWS * v0.Dot(m_Drag.planeNormalWS);

                const float len = v0.Length();
                if (len < 1e-4f)
                    return false;

                m_Drag.uniformDirWS = v0 / len;
                m_Drag.bHasUniformDir = true;
            }

            if (IsScalePlane(m_Active))
            {
                // Choose plane axes (U,V) in gizmo basis
                switch (m_Active)
                {
                    case EHandle::S_XY: m_Drag.planeAxisU = m_Drag.X; m_Drag.planeAxisV = m_Drag.Y; break;
                    case EHandle::S_XZ: m_Drag.planeAxisU = m_Drag.X; m_Drag.planeAxisV = m_Drag.Z; break;
                    case EHandle::S_YZ: m_Drag.planeAxisU = m_Drag.Y; m_Drag.planeAxisV = m_Drag.Z; break;
                    default: break;
                }

                const FVector3 d0 = m_Drag.startHitWS - m_Drag.pivotWS;
                m_Drag.planeU0 = d0.Dot(m_Drag.planeAxisU);
                m_Drag.planeV0 = d0.Dot(m_Drag.planeAxisV);
                m_Drag.bHasPlaneScaleStart = true;
            }
        }
    }

    // Axis-param start (stable for axis translate/scale)
    if (IsTranslateAxis(m_Active) || IsScaleAxis(m_Active))
    {
        float s0 = 0.0f;
        if (RayAxisClosestS(rayWS, m_Drag.pivotWS, m_Drag.axisWS, s0))
        {
            m_Drag.bHasAxisSStart = true;
            m_Drag.axisSStart = s0;
        }
    }

    // Validate required start data
    if (IsTranslateCenter(m_Active) && !m_Drag.bHasStartHit) return false;
    if (IsTranslatePlane(m_Active)  && !m_Drag.bHasStartHit) return false;
    if (IsRotateHandle(m_Active)    && !m_Drag.bHasStartHit) return false;

    if (IsScaleUniform(m_Active) && (!m_Drag.bHasStartHit || !m_Drag.bHasUniformDir)) return false;
    if (IsScalePlane(m_Active)   && (!m_Drag.bHasStartHit || !m_Drag.bHasPlaneScaleStart)) return false;

    return true;
}

bool GizmoEditorController::UpdateDrag(const FRay& rayWS, const FVector3& /*camFwd*/, FGizmoTransformDelta& outDelta)
{
    if (!m_Drag.bActive) return false;

    outDelta = {};
    outDelta.mode    = m_Drag.mode;
    outDelta.space   = m_Drag.space;
    outDelta.handle  = m_Drag.handle;
    outDelta.pivotWS = m_Drag.pivotWS;

    // ----- TRANSLATE -----
    if (m_Drag.mode == GizmoEditorTool::EMode::Translate)
    {
        if (IsTranslatePlane(m_Drag.handle))
        {
            FVector3 hit{};
            if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit))
                return true;

            FVector3 delta = hit - m_Drag.startHitWS;
            delta -= m_Drag.planeNormalWS * delta.Dot(m_Drag.planeNormalWS);

            outDelta.deltaTranslationWS = delta;
            outDelta.bHasDelta = true;
            return true;
        }

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

            FVector3 hit{};
            if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit) || !m_Drag.bHasStartHit)
                return true;

            const float ds = (hit - m_Drag.startHitWS).Dot(m_Drag.axisWS);
            outDelta.deltaTranslationWS = m_Drag.axisWS * ds;
            outDelta.bHasDelta = true;
            return true;
        }

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
    if (m_Drag.mode == GizmoEditorTool::EMode::Rotate && IsRotateHandle(m_Drag.handle))
    {
        FVector3 hit{};
        if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit))
            return true;

        FVector3 v = hit - m_Drag.pivotWS;
        const float len = v.Length();
        if (len < 1e-6f) return true;

        const FVector3 vCur = v / len;

        if (m_Drag.bSkipFirstRotateUpdate)
        {
            // Re-anchor rotation to *current* cursor sample so no snap happens.
            m_Drag.prevVecWS = vCur;
            m_Drag.angleAccumRad = 0.0f;
            m_Drag.bSkipFirstRotateUpdate = false;

            // Don’t output a delta this frame.
            return true;
        }

        const float dAng = SignedAngleAroundAxis(m_Drag.prevVecWS, vCur, m_Drag.axisWS);
        m_Drag.angleAccumRad += dAng;
        m_Drag.prevVecWS = vCur;

        outDelta.rotationAxisWS  = m_Drag.axisWS;
        outDelta.rotationAngleRad = m_Drag.angleAccumRad;
        outDelta.bHasDelta = true;
        return true;
    }

    // ----- SCALE -----
    if (m_Drag.mode == GizmoEditorTool::EMode::Scale)
    {
        FVector3 hit{};
        if (!RayPlaneThroughPoint(rayWS, m_Drag.pivotWS, m_Drag.planeNormalWS, hit) || !m_Drag.bHasStartHit)
            return true;

        if (IsScaleAxis(m_Drag.handle))
        {
            const float ds = (hit - m_Drag.startHitWS).Dot(m_Drag.axisWS);

            float axisMul = 1.0f + ds * m_Cfg.scaleSensitivity;
            axisMul = std::max(axisMul, m_Cfg.minAxisScaleMul);

            FVector3 mul(1,1,1);
            switch (m_Drag.handle)
            {
                case EHandle::S_X: mul.x = axisMul; break;
                case EHandle::S_Y: mul.y = axisMul; break;
                case EHandle::S_Z: mul.z = axisMul; break;
                default: break;
            }

            outDelta.deltaScaleMul = mul;
            outDelta.bHasDelta = true;
            return true;
        }

        if (IsScaleUniform(m_Drag.handle))
        {
            const float ds = (hit - m_Drag.startHitWS).Dot(m_Drag.uniformDirWS);

            float u = 1.0f + ds * m_Cfg.scaleSensitivity;
            u = std::max(u, m_Cfg.minAxisScaleMul);

            outDelta.deltaScaleMul = FVector3(u,u,u);
            outDelta.bHasDelta = true;
            return true;
        }

        if (IsScalePlane(m_Drag.handle))
        {
            if (!m_Drag.bHasPlaneScaleStart) return true;

            const FVector3 d = hit - m_Drag.pivotWS;

            const float u = d.Dot(m_Drag.planeAxisU);
            const float v = d.Dot(m_Drag.planeAxisV);

            const float du = u - m_Drag.planeU0;
            const float dv = v - m_Drag.planeV0;

            // Use diagonal drag distance on the plane
            float ds = std::sqrt(du*du + dv*dv);

            // Preserve a sign so dragging “out” grows and “in” shrinks.
            // This uses the dominant axis sign to avoid jitter near diagonal.
            const float sign = (std::fabs(du) > std::fabs(dv)) ? (du >= 0.f ? 1.f : -1.f)
                                                               : (dv >= 0.f ? 1.f : -1.f);
            ds *= sign;

            float s = 1.0f + ds * m_Cfg.scaleSensitivity;
            s = std::max(s, m_Cfg.minAxisScaleMul);

            FVector3 mul(1,1,1);
            switch (m_Drag.handle)
            {
                case GizmoEditorTool::EHandle::S_XY: mul.x = s; mul.y = s; break;
                case GizmoEditorTool::EHandle::S_XZ: mul.x = s; mul.z = s; break;
                case GizmoEditorTool::EHandle::S_YZ: mul.y = s; mul.z = s; break;
                default: break;
            }

            outDelta.deltaScaleMul = mul;
            outDelta.bHasDelta = true;
            return true;
        }

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

            const bool bDrag = BeginDrag(rayWS, camFwd, gizmoXf);
            if (!bDrag)
            {
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
        m_Hovered = GizmoEditorTool::EHandle::None;
        result.bWantsCapture = true;
        result.bConsumesClick = true;

        UpdateDrag(rayWS, camFwd, result.manipulation);

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
    finalP.bDimOthersWhenActive = m_bCapturing;
    finalP.scaleMul = ComputeGizmoScaleMulFromProjection(projMat, view.viewportH);
    finalP.alphaMul = ComputeFadeMul(viewMat, gizmoXf.GetPosition());

    m_GizmoTool.Draw(debugDraw, view.viewIndex, camPos, camFwd, gizmoXf, finalP);

    result.hovered = m_Hovered;
    result.active  = m_Active;
    return result;
}
