//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>

#include "Core/Math/FVector3.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FTransform.h"
#include "Framework/DebugDrawFramework.h"

class GizmoEditorTool
{
public:
    enum class EMode : uint8_t { Translate, Rotate, Scale };
    enum class ESpace : uint8_t { Local, World };

    enum class EHandle : uint8_t
    {
        None = 0,

        // Translate
        T_X,
        T_Y,
        T_Z,
        T_XY,
        T_XZ,
        T_YZ,
        T_Center,

        // Rotate
        R_X,
        R_Y,
        R_Z,
        R_Free,

        // Scale
        S_X,
        S_Y,
        S_Z,
        S_Uniform
    };

    struct FDrawParams
    {
        EMode  mode  = EMode::Translate;
        ESpace space = ESpace::World;

        // Base hit id for this panel (unique per panel).
        // All handles derive from this.
        uint32_t baseHitID = 0;

        // Highlighting (provided by editor once you implement hit testing)
        EHandle hoveredHandle = EHandle::None;
        EHandle activeHandle  = EHandle::None;

        // Visual toggles
        bool bDrawCenter  = true;   // center sphere/cube
        bool bDrawPlanes  = true;   // translate plane handles
        bool bDrawSphereHint = true; // rotate: faint sphere

        float scaleMul = 1.0f;
        float alphaMul = 1.0f;
    };

public:
    GizmoEditorTool() = default;

    // Main: render gizmo primitives into DebugDraw.
    // camPos/camFwd are used for constant-size scaling and plane handle orientation if needed later.
    void Draw(DebugDraw& dd,
              uint32_t viewKey,
              const FVector3& camPos,
              const FVector3& camFwd,
              const FTransform& gizmoXf,
              const FDrawParams& p);

    // Handle <-> hitId mapping
    [[nodiscard]] uint32_t HandleToHitId(uint32_t baseHitId, EHandle h) const;
    [[nodiscard]] EHandle HitIdToHandle(uint32_t baseHitId, uint32_t hitId) const;

private:
    // Visual tuning (in "gizmo units" before scaling)
    struct FVisualConfig
    {
        float axisLen      = 1.5f;
        float headLen      = 0.3f;  // translate arrow head
        float headRadius   = 0.10f;

        float centerRadius = 0.1f;  // translate center sphere
        float planeSize    = 0.25f;  // translate plane square half size-ish
        float planeOffset  = 0.9f;  // how far from origin plane handle sits

        float ringRadius   = 1.3f;   // rotate ring radius
        float ringThicknessPx = 5.0f;
        int   ringSegments = 64;

        float scaleArmLen = 1.1f;
        float scaleBoxHalf = 0.09f;  // axis box head half extent
        float uniformBoxHalf = 0.10f;
        float scaleArmThicknessPx = 2.0f;

        float axisThicknessPx = 2.0f;
        float highlightMul = 1.35f;  // brighten hovered/active colors
        float activeMul    = 1.65f;
        float planeAlpha   = 0.4f;
        float sphereHintAlpha = 0.01f;

        EDebugShading shading = EDebugShading::FixedLit;
        EDebugNormalMode normalMode = EDebugNormalMode::Flat;
    };

private:
    static FVector4 Brighten(const FVector4& c, float mul);
    static float ComputeGizmoScale(const FVector3& camPos, const FVector3& gizmoPos);

    static void BuildBasis(const FTransform& xf, ESpace space,
                           FVector3& outX, FVector3& outY, FVector3& outZ);

    void DrawTranslate(DebugDraw& dd,
                       uint32_t viewKey,
                       const FVector3& camPos,
                       const FVector3& camFwd,
                       const FTransform& xf,
                       const FDrawParams& p,
                       float gizmoScale,
                       const FVisualConfig& v) const;

    void DrawRotate(DebugDraw& dd,
                    uint32_t viewKey,
                    const FVector3& camPos,
                    const FVector3& camFwd,
                    const FTransform& xf,
                    const FDrawParams& p,
                    float gizmoScale,
                    const FVisualConfig& v) const;

    void DrawScale(DebugDraw& dd,
                   uint32_t viewKey,
                   const FVector3& camPos,
                   const FVector3& camFwd,
                   const FTransform& xf,
                   const FDrawParams& p,
                   float gizmoScale,
                   const FVisualConfig& v) const;

    // Utility for plane squares made of 4 lines (wireframe).
    static void DrawPlaneSquareWire(DebugDraw& dd,
                                    const FVector3& origin,
                                    const FVector3& axisA,
                                    const FVector3& axisB,
                                    float halfSize,
                                    const FVector4& color,
                                    const FDebugDrawStyle& style);

    static void DrawPlaneSquareSolid(DebugDraw& dd,
                                 const FVector3& origin,
                                 const FVector3& axisA,
                                 const FVector3& axisB,
                                 float halfSize,
                                 const FVector4& color,
                                 FDebugDrawStyle style);
};
