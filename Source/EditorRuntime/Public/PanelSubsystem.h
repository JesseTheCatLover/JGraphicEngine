//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <memory>

#include "PanelRegistry.h"
#include "Utilities/UDynamicID.h"
#include "Tools/TEditorTools.h"
#include "Tools/CameraEditorTool.h"
#include "Tools/GizmoEditorTool.h"

struct FViewportPanelInput;
class EditorHost;
class EditorRuntime;
class ViewportController;

class PanelSubsystem
{
private:
    EditorHost& m_Core;
    EditorRuntime& m_Runtime;

    PanelRegistry m_PanelIds;

    // Tool pools
    TEditorTools<CameraEditorTool> m_CameraTools;
    TEditorTools<GizmoEditorTool>  m_GizmoTools;

    // One controller per panel
    std::unordered_map<PanelID, std::unique_ptr<ViewportController>> m_Viewports;

    // Latest frame snapshot per panel (submitted by panels)
    std::unordered_map<PanelID, FViewportPanelInput> m_LatestContexts;

private:
    ViewportController& GetOrCreateViewport(PanelID panelId);

public:
    PanelSubsystem(EditorHost& core, EditorRuntime& runtime);
    ~PanelSubsystem();

    // Called once per frame from Core
    void Tick(float dt);

    // Called from Core (panel bridge)
    void SubmitViewportInput(const FViewportPanelInput& input);

    [[nodiscard]] void* GetViewportNativeTexture(const char* panelKey) const;

    void DestroyViewport(const char* panelKey);

    // ---- Tool instance ownership (used by controllers) ----
    UDynamicID::IDType CreateCameraTool();
    bool DestroyCameraTool(UDynamicID::IDType id);
    CameraEditorTool* GetCameraTool(UDynamicID::IDType id);

    UDynamicID::IDType CreateGizmoTool();
    bool DestroyGizmoTool(UDynamicID::IDType id);
    GizmoEditorTool* GetGizmoTool(UDynamicID::IDType id);
};
