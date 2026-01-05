//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Tools/Controllers/GizmoEditorController.h"
#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Rendering/FRenderView.h"

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
        hMul = std::clamp(hMul, 0.6f, 1.8f); // tune clamp
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
        result.active  = m_Active;
        return result;
    }

    // 1) Decide hovered handle (only when not capturing)
    GizmoEditorTool::EHandle newHover = GizmoEditorTool::EHandle::None;
    if (input.bFocused && !m_bCapturing && input.bOverViewport)
        newHover = HitTest(debugDraw, view, viewMat, projMat, input.mouseX, input.mouseY);

    // 2) Begin capture (based on current hover) — only if allowed
    if (!m_bCapturing)
    {
        if (bAllowBeginCapture && input.bOverViewport && input.bLeftClicked &&
            newHover != GizmoEditorTool::EHandle::None)
        {
            m_bCapturing = true;
            m_Active = newHover;


            result.bWantsCapture  = true;
            result.bConsumesClick = m_Cfg.bGizmoFirst;
        }
    }
    else
    {
        result.bWantsCapture = true;
        if (input.bLeftReleased)
            CancelCapture();
    }

    // 3) Commit hover after decisions
    if (!m_bCapturing)
        m_Hovered = newHover;
    else
        m_Hovered = GizmoEditorTool::EHandle::None;

    // 4) Draw gizmo using current state
    GizmoEditorTool::FDrawParams params{};
    params.mode         = m_Mode;
    params.space        = m_Space;
    params.baseHitID    = m_BaseHitID;
    params.hoveredHandle= m_Hovered;
    params.activeHandle = m_Active;
    params.bDrawSphereHint = true;

    params.scaleMul = ComputeGizmoScaleMulFromProjection(projMat, view.viewportH);
    params.alphaMul = ComputeFadeMul(viewMat, gizmoXf.GetPosition());

    m_GizmoTool.Draw(debugDraw, view.viewIndex, camPos, camFwd, gizmoXf, params);

    result.hovered = m_Hovered;
    result.active  = m_Active;
    result.bWantsCapture = result.bWantsCapture || m_bCapturing;

    return result;
}

float GizmoEditorController::ComputeFadeMul(const FMatrix4 &viewMat, const FVector3 &gizmoPosWS)
{
    // view-space position
    const FVector4 pVS4 = viewMat * FVector4(gizmoPosWS.x, gizmoPosWS.y, gizmoPosWS.z, 1.0f);
    const float zVS = std::fabs(pVS4.x); // LH: X forward => depth is X in engine's convention (camera forward is +X)

    // If depth is too small, fade out
    // Tune these as "meters" in view space.
    constexpr float kFadeStart = 3.f;
    constexpr float kFadeEnd   = 0.3f;

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
}
