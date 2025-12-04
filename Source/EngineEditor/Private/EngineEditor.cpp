//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EngineEditor.h"

#include <vector>
#include <unordered_map>
#include "Core/EngineContext.h"
#include "Core/JEngine.h"
#include "Tools/CameraEditorTool.h"

EngineEditor::EngineEditor()
    : m_Context(JEngine::Get().GetEngineContext())
    , m_SceneManager(*JEngine::Get().GetSceneManager())
    , m_Renderer(*JEngine::Get().GetRenderer())
    , m_PlatformSurface(*JEngine::Get().GetPlatformSurface())
    , m_SceneAPI(m_Context, m_SceneManager)
    , m_ViewportAPI(m_Context, m_Renderer)
    , m_SurfaceAPI(m_Context, m_PlatformSurface)
{
    // Editor takes over rendering, so don't render directly to platform surface
    m_Context.SetShouldRenderToPlatformSurface(false);
}

EngineEditor::~EngineEditor()
{

}

void EngineEditor::TickAllTools(float deltaTime, const FEditorToolFrameState& state)
{
    TickCameraTools(deltaTime, state);
}

void EngineEditor::SubmitEditorViewSources(const FEditorToolFrameState& state) // TODO: TEMP
{
    ICameraViewSource* camera = nullptr;
    float aspect = 16.f / 9.f;

    if (state.activeCameraId != UDynamicID::InvalidID)
    {
        if (CameraEditorTool* tool = m_CameraTools.Get(state.activeCameraId))
        {
            camera = tool;

            auto it = state.cameraAspectMap.find(state.activeCameraId);
            if (it != state.cameraAspectMap.end())
                aspect = it->second;
        }
    }

    // Fallback: if no active camera, either:
    // - leave camera as nullptr (renderer logs and bails), or
    // - pick the first camera tool as a default.
    if (!camera)
    {
        auto views = CollectEditorCameraViews();
        if (!views.empty())
        {
            camera = views.front();
            // aspect stays default or you can guess something better.
        }
    }

    // Push the result into EngineContext so RendererSubsystem can use it.
    m_Context.SetCamera(camera, aspect);
}

UDynamicID::IDType EngineEditor::CreateCameraEditorTool()
{
    return m_CameraTools.Create();
}

bool EngineEditor::DestroyCameraEditorTool(UDynamicID::IDType cameraID)
{
    return m_CameraTools.Destroy(cameraID);
}

CameraEditorTool* EngineEditor::GetCameraEditorTool(UDynamicID::IDType cameraID)
{
    return m_CameraTools.Get(cameraID);
}

void EngineEditor::TickCameraTools(float deltaTime, const FEditorToolFrameState& state)
{
    m_CameraTools.ForEach(
        [&](UDynamicID::IDType id, CameraEditorTool& tool)
        {
            const bool isActive = (id == state.activeCameraId);
            float aspect = 16.f / 9.f;

            if (auto it = state.cameraAspectMap.find(id); it != state.cameraAspectMap.end())
                aspect = it->second;

            tool.Tick(deltaTime, isActive, aspect);
        });
}

std::vector<ICameraViewSource*> EngineEditor::CollectEditorCameraViews() const
{
    std::vector<ICameraViewSource*> outViews;
    m_CameraTools.ForEach(
        [&](UDynamicID::IDType id, const CameraEditorTool& tool)
        {
            // CameraEditorTool is an ICameraViewSource
            outViews.push_back(const_cast<CameraEditorTool*>(&tool));
        });
    return outViews;
}
