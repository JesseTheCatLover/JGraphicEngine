//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Tools/TEditorTools.h"
#include "Tools/CameraEditorTool.h"
#include "Tools/GizmoEditorTool.h"
#include "Utilities/UDynamicID.h"

class ToolService
{
private:
    TEditorTools<CameraEditorTool> m_CameraTools;
    TEditorTools<GizmoEditorTool>  m_GizmoTools;

public:
    // Camera
    UDynamicID::IDType CreateCameraTool() { return m_CameraTools.Create(); }
    bool DestroyCameraTool(UDynamicID::IDType id) { return m_CameraTools.Destroy(id); }
    CameraEditorTool* GetCameraTool(UDynamicID::IDType id) { return m_CameraTools.Get(id); }

    // Gizmo
    UDynamicID::IDType CreateGizmoTool() { return m_GizmoTools.Create(); }
    bool DestroyGizmoTool(UDynamicID::IDType id) { return m_GizmoTools.Destroy(id); }
    GizmoEditorTool* GetGizmoTool(UDynamicID::IDType id) { return m_GizmoTools.Get(id); }
};
