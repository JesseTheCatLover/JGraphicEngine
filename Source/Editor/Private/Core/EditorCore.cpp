//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EditorCore.h"
#include "EditorContext.h"
#include <algorithm>
#include <iostream>

#include "Core/EngineGlobals.h"
#include "Core/JEngine.h"
#include "Core/Math/FMatrix4.h"
#include "Core/Math/FVector4.h"
#include "Framework/SceneManager.h"
#include "Rendering/IPlatformSurface.h"
#include "Tools/CameraEditorTool.h"

EditorCore::EditorCore(EditorContext &context, EngineEditor& engineEditor):
m_Context(context),
m_EngineEditor(engineEditor)
{
}

void EditorCore::Tick(float deltaTime)
{
    PushFrameInfoToEditorContext();

    TickEditorTools(deltaTime);

    ClearFrameStates();
}

void EditorCore::PushFrameInfoToEditorContext()
{
    // Pull frame info to EditorContext
    m_FrameSnapshot = m_EngineEditor.GetViewportAPI().GetFrameSnapshot();
    m_Context.SetFrameSnapshot(m_FrameSnapshot);
}

void EditorCore::TickEditorTools(float deltaTime)
{
    FEditorToolFrameState toolState;

    TickCameraTools(toolState.camera);

    // Tick tools in engine editor
    m_EngineEditor.TickAllTools(deltaTime, toolState);

    // Build views
    m_EngineEditor.SubmitEditorViewSources(toolState.camera);
}

void EditorCore::TickCameraTools(FCameraToolState& cameraState)
{
    // 1) Active camera (panel with mouse capture)
    if (m_ActiveViewportPanel)
    {
        auto itCam = m_PanelToCameraMap.find(m_ActiveViewportPanel);
        if (itCam != m_PanelToCameraMap.end())
        {
            cameraState.activeCameraId = itCam->second;
        }
    }

    // 2) Build viewstateMap from per-panel size
    int viewIndex = 0;
    for (const auto& [panel, camId] : m_PanelToCameraMap)
    {
        auto itVp = m_CameraStateMap.find(panel);
        if (itVp == m_CameraStateMap.end())
            continue;

        const auto& vp = itVp->second;
        if (vp.width <= 0.f || vp.height <= 0.f)
            continue;

        FCameraViewState vs{};
        vs.width  = vp.width;
        vs.height = vp.height;
        vs.aspect = (vp.height > 0.f)
                        ? (vp.width / vp.height)
                        : 16.f / 9.f;
        vs.viewIndex = viewIndex++;

        cameraState.viewstateMap[camId] = vs;
    }

    // 3) Propagate MSAA samples
    for (const auto& [camId, samples] : m_CameraSampleMap)
    {
        cameraState.cameraSampleMap[camId] = samples;
    }
}

void EditorCore::ClearFrameStates()
{
}

void* EditorCore::GetViewportTextureHandle(const IEditorPanel* panel) const
{
    auto itCam = m_PanelToCameraMap.find(panel);
    if (itCam == m_PanelToCameraMap.end())
        return nullptr;

    UDynamicID::IDType camId = itCam->second;

    RTextureHandle color = m_EngineEditor.GetViewportColorHandle(camId);
    if (!color.IsValid())
        return nullptr;

    return m_EngineEditor.GetNativeTextureHandle(color);
}

void EditorCore::PickActorAtViewportPos(const IEditorPanel* panel, float x, float y)
{
    // 1) Find the camera associated with this panel
    auto camIt = m_PanelToCameraMap.find(panel);
    if (camIt == m_PanelToCameraMap.end())
        return;

    UDynamicID::IDType cameraID = camIt->second;
    CameraEditorTool* cameraTool = m_EngineEditor.GetCameraEditorTool(cameraID);
    if (!cameraTool)
        return;

    // 2) Get viewport size for this panel
    auto vpIt = m_CameraStateMap.find(panel);
    if (vpIt == m_CameraStateMap.end())
        return;

    const float width  = vpIt->second.width;
    const float height = vpIt->second.height;
    if (width <= 0.f || height <= 0.f)
        return;

    // 3) Convert local pixel coords -> NDC [-1,1]
    // x,y are in [0..width],[0..height] with (0,0) at top-left of the Image.
    const float xNDC =  2.0f * (x / width)  - 1.0f;
    const float yNDC =  1.0f - 2.0f * (y / height); // convert top-left to NDC

    const float aspect = width / height;

    // 4) Common camera data
    const FQuat camRot = cameraTool->GetRotation();
    const FVector3 camPos = cameraTool->GetPosition();

    FVector3 originWorld;
    FVector3 dirWorld;

    if (cameraTool->GetProjectionType() == EProjectionType::Perspective)
    {
        // ---------------------------
        // Perspective ray
        // Left-handed, X = forward, Y = right, Z = up
        // ---------------------------
        const float verticalFovDeg = cameraTool->GetFOV();
        const float halfFovRad = verticalFovDeg * 0.5f * (3.1415926535f / 180.0f);
        const float tanHalfFov = std::tan(halfFovRad);

        // Point on a plane 1 unit in front of the camera (camera space)
        FVector3 dirCam;
        dirCam.x = 1.0f;                        // forward (X)
        dirCam.y = xNDC * tanHalfFov * aspect;  // right   (Y)
        dirCam.z = yNDC * tanHalfFov;           // up      (Z)
        dirCam   = dirCam.Normalized();

        // Rotate into world space
        dirWorld  = camRot.RotateVector(dirCam).Normalized();
        originWorld = camPos; // ray starts at camera position
    }
    else
    {
        // ---------------------------
        // Orthographic ray
        // Direction is constant (camera forward), origin slides in the view plane
        // ---------------------------

        // Ortho volume in camera space:
        //  - vertical half-size is m_OrthoHalfHeight
        //  - horizontal half-size = halfHeight * aspect
        const float halfHeight = cameraTool->GetOrthoHalfHeight();
        const float halfWidth = halfHeight * aspect;

        // NDC -> camera-space offsets on the view plane
        // In camera space: X = forward, Y = right, Z = up
        FVector3 pointCam;
        pointCam.x = 0.0f;  // on plane through camera, forward handled by dir
        pointCam.y = xNDC * halfWidth; // right offset
        pointCam.z = yNDC * halfHeight; // up offset

        // Transform this point into world
        const FVector3 pointWorld = camRot.RotateVector(pointCam) + camPos;

        // Forward direction in camera space is +X
        const FVector3 forwardCam(1.0f, 0.0f, 0.0f);
        dirWorld = camRot.RotateVector(forwardCam).Normalized();

        originWorld = pointWorld;
    }

    // 5) Build ray
    FRay ray;
    ray.origin = originWorld;
    ray.direction = dirWorld;

    // 6) Raycast
    FRaycastHit hit{};
    if (m_EngineEditor.GetSceneAPI().Raycast(ray, hit) && hit.bHit)
    {
        SelectActor(static_cast<int>(hit.actorID));
    }
    else
    {
        // Optional: clear selection
    }
}

void EditorCore::CreateCameraForPanel(const IEditorPanel* panel)
{
    if (m_PanelToCameraMap.count(panel) > 0)
        return;

    auto id = m_EngineEditor.CreateCameraEditorTool();
    m_PanelToCameraMap[panel] = id;
}

void EditorCore::DestroyCameraForPanel(const IEditorPanel* panel)
{
    auto it = m_PanelToCameraMap.find(panel);
    if (it == m_PanelToCameraMap.end())
        return;

    UDynamicID::IDType id = it->second;

    m_EngineEditor.DestroyCameraEditorTool(id);

    m_PanelToCameraMap.erase(it);
    m_CameraStateMap.erase(panel);
    DeactivateCameraForPanel(panel);
}

void EditorCore::SetViewportFocused(const IEditorPanel *panel, bool bFocused)
{
    IPlatformSurface* surface = JEngine::Get().GetPlatformSurface();
    if (bFocused)
    {
        if (surface)
            surface->SetCursorMode(ECursorMode::Disabled);

        ActivateCameraForPanel(panel);
    }
    else
    {
        if (surface)
            surface->SetCursorMode(ECursorMode::Visible);

        DeactivateCameraForPanel(panel);
    }
}

void EditorCore::ActivateCameraForPanel(const IEditorPanel *panel)
{
    m_ActiveViewportPanel = panel;
}

void EditorCore::DeactivateCameraForPanel(const IEditorPanel *panel)
{
    if (m_ActiveViewportPanel == panel)
        m_ActiveViewportPanel = nullptr;
}

void EditorCore::OnViewportResized(const IEditorPanel *panel, float width, float height)
{
    auto it = m_PanelToCameraMap.find(panel);
    if (it == m_PanelToCameraMap.end())
    {
        // If panel somehow resized before OnCreate, make a camera now.
        CreateCameraForPanel(panel);
        it = m_PanelToCameraMap.find(panel);
        if (it == m_PanelToCameraMap.end())
            return;
    }

    FViewportPanelState vp{};
    vp.width = width;
    vp.height = height;

    m_CameraStateMap[panel] = vp;
}

void EditorCore::SetViewportMSAASamples(const IEditorPanel *panel, int samples)
{
    auto it = m_PanelToCameraMap.find(panel);
    if (it == m_PanelToCameraMap.end())
        return;

    const auto id = it->second;

    // Clamp to sane values
    if (samples < 1) samples = 1;
    if (samples > 8) samples = 8;

    m_CameraSampleMap[id] = samples;
}

void EditorCore::ExecuteCommand(TUniquePtr<IEditorCommand> cmd)
{
    if (!cmd)
        return;

    cmd->Apply(m_Context);

    m_UndoStack.push(std::move(cmd));

    // Once a new command is executed, redo history is invalid.
    while (!m_RedoStack.empty())
        m_RedoStack.pop();
}

void EditorCore::Undo()
{
    if (m_UndoStack.empty())
        return;

    auto cmd = TakeUniqueOwnership(m_UndoStack.top());
    m_UndoStack.pop();

    cmd->Undo(m_Context);

    m_RedoStack.push(TakeUniqueOwnership(cmd));
}

void EditorCore::Redo()
{
    if (m_RedoStack.empty())
        return;

    auto cmd = TakeUniqueOwnership(m_RedoStack.top());
    m_RedoStack.pop();

    cmd->Apply(m_Context);

    m_UndoStack.push(TakeUniqueOwnership(cmd));
}

void EditorCore::SelectActor(int actorId)
{
    if (actorId == -1)
        return;

    if (auto selectedActor = GetSceneManager()->GetActiveScene()->FindActorByID(actorId))
    {
        std::cout << actorId << std::endl;
        std::cout << "Selecting actor " + selectedActor->GetName() << std::endl;
    }
}