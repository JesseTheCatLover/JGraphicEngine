//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "PanelSubsystem.h"

#include "Core/EditorHost.h"
#include "EditorRuntime.h"
#include "Controllers/ViewportController.h"
#include "Controllers/Inputs/FViewportPanelInput.h"

PanelSubsystem::PanelSubsystem(EditorHost& core, EditorRuntime& runtime)
    : m_Core(core)
    , m_Runtime(runtime)
{
}

PanelSubsystem::~PanelSubsystem()
{
    m_Viewports.clear(); // Destroy controllers first (they destroy tools)
}

ViewportController& PanelSubsystem::GetOrCreateViewport(PanelID panelId)
{
    auto it = m_Viewports.find(panelId);
    if (it != m_Viewports.end())
        return *it->second;

    auto vc = std::make_unique<ViewportController>(panelId, m_Core, m_Runtime, *this);
    ViewportController& ref = *vc;
    m_Viewports.emplace(panelId, std::move(vc));
    return ref;
}

void PanelSubsystem::SubmitViewportInput(const FViewportPanelInput& input)
{
    if (!input.panelKey || input.panelKey[0] == '\0')
        return;

    PanelID pid = m_PanelIds.GetOrCreate(input.panelKey);

    m_LatestContexts[pid] = input;
    (void)GetOrCreateViewport(pid);
}

void* PanelSubsystem::GetViewportNativeTexture(const char* panelKey) const
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

void PanelSubsystem::DestroyViewport(const char* panelKey)
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

void PanelSubsystem::Tick(float dt)
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
UDynamicID::IDType PanelSubsystem::CreateCameraTool()
{
    return m_CameraTools.Create();
}

bool PanelSubsystem::DestroyCameraTool(UDynamicID::IDType id)
{
    return m_CameraTools.Destroy(id);
}

CameraEditorTool* PanelSubsystem::GetCameraTool(UDynamicID::IDType id)
{
    return m_CameraTools.Get(id);
}

UDynamicID::IDType PanelSubsystem::CreateGizmoTool()
{
    return m_GizmoTools.Create();
}

bool PanelSubsystem::DestroyGizmoTool(UDynamicID::IDType id)
{
    return m_GizmoTools.Destroy(id);
}

GizmoEditorTool* PanelSubsystem::GetGizmoTool(UDynamicID::IDType id)
{
    return m_GizmoTools.Get(id);
}
