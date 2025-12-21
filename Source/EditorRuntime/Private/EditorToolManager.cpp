//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EditorToolManager.h"

#include "Core/EditorCore.h"
#include "EditorRuntime.h"
#include "Controllers/ViewportController.h"

EditorToolManager::EditorToolManager(EditorCore& core, EditorRuntime& runtime)
    : m_Core(core)
    , m_Runtime(runtime)
{
}

EditorToolManager::~EditorToolManager()
{
    m_Viewports.clear(); // Destroy controllers first (they destroy tools)
}

ViewportController& EditorToolManager::GetOrCreateViewport(PanelID panelId)
{
    auto it = m_Viewports.find(panelId);
    if (it != m_Viewports.end())
        return *it->second;

    auto vc = std::make_unique<ViewportController>(panelId, m_Core, m_Runtime, *this);
    ViewportController& ref = *vc;
    m_Viewports.emplace(panelId, std::move(vc));
    return ref;
}

void EditorToolManager::SubmitViewportPanelContext(const FViewportPanelContext& ctx)
{
    if (!ctx.panelKey || ctx.panelKey[0] == '\0')
        return;

    PanelID pid = m_PanelIds.GetOrCreate(ctx.panelKey);

    m_LatestContexts[pid] = ctx;
    (void)GetOrCreateViewport(pid);
}

void* EditorToolManager::GetViewportNativeTexture(const char* panelKey) const
{
    if (!panelKey || panelKey[0] == '\0')
        return nullptr;

    PanelID pid = m_PanelIds.Find(panelKey);
    if (pid == UDynamicID::InvalidID)
        return nullptr;

    auto it = m_Viewports.find(pid);
    if (it == m_Viewports.end())
        return nullptr;

    return it->second->GetNativeTexture();
}

void EditorToolManager::DestroyViewport(const char* panelKey)
{
    if (!panelKey || panelKey[0] == '\0')
        return;

    PanelID pid = m_PanelIds.Find(panelKey);
    if (pid == UDynamicID::InvalidID)
        return;

    m_LatestContexts.erase(pid);
    m_Viewports.erase(pid);

    m_PanelIds.Release(panelKey);
}

void EditorToolManager::Tick(float dt)
{
    // Update each viewport controller using its latest submitted frame
    for (auto& [panelId, ctrl] : m_Viewports)
    {
        auto it = m_LatestContexts.find(panelId);
        if (it == m_LatestContexts.end())
            continue; // panel didn't submit this frame

        ctrl->Update(dt, it->second);
    }
}

// ---- Tool pools ----
UDynamicID::IDType EditorToolManager::CreateCameraTool()
{
    return m_CameraTools.Create();
}

bool EditorToolManager::DestroyCameraTool(UDynamicID::IDType id)
{
    return m_CameraTools.Destroy(id);
}

CameraEditorTool* EditorToolManager::GetCameraTool(UDynamicID::IDType id)
{
    return m_CameraTools.Get(id);
}

UDynamicID::IDType EditorToolManager::CreateGizmoTool()
{
    return m_GizmoTools.Create();
}

bool EditorToolManager::DestroyGizmoTool(UDynamicID::IDType id)
{
    return m_GizmoTools.Destroy(id);
}

GizmoEditorTool* EditorToolManager::GetGizmoTool(UDynamicID::IDType id)
{
    return m_GizmoTools.Get(id);
}
