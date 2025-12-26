//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "PanelRegistry.h"
#include "Rendering/FViewportRT.h"
#include "Utilities/UDynamicID.h"
#include "Rendering/RHandles.h"

struct FViewportPanelInput;
struct FViewportOutput;

class EditorHost;
class EditorRuntime;
class ToolService;

class ViewportController
{
private:
    PanelID m_PanelID = 0;

    EditorHost&    m_Host;
    EditorRuntime& m_Runtime;
    ToolService&   m_Tools;

    // One camera tool per panel
    UDynamicID::IDType m_CameraToolID = 0;

    // Controller-owned RT
    FViewportRT m_RT;

    int m_PostProfile = 1;

    bool m_bHasMouseCapture = false;

    // Cached viewport facts (from input)
    float m_Width = 0.f;
    float m_Height = 0.f;
    bool m_Focused = false;
    bool m_Hovered = false;

    int m_MSAASamples = 4;

private:
    void EnsureCameraTool();
    void DestroyCameraTool();

    void EnsureRenderTarget();
    void DestroyRenderTarget();

    void TickCamera(float dt);
    void SubmitView();

    void UpdateCapturePolicy(const FViewportPanelInput& input);

public:
    ViewportController(PanelID id, EditorHost& host, EditorRuntime& runtime, ToolService& tools);
    ~ViewportController();

    void Update(float deltaTime, const FViewportPanelInput& input, FViewportOutput& out);

    void OnPanelDestroyed();
};
