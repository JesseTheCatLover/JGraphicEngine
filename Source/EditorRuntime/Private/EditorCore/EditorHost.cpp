//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorCore/EditorHost.h"

#include "Panels/TPanelContainer.h"
#include "EditorCore/DialogManager.h"
#include "Tools/ToolService.h"
#include "EditorCore/Services/AssetBrowser/AssetBrowserService.h"
#include "EditorCore/Services/EditTimelineService.h"
#include "EditorCore/Services/HierarchyService.h"
#include "EditorCore/Services/HotkeyService.h"
#include "EditorCore/Services/ScenePickingService.h"
#include "EditorCore/Services/SceneQueryService.h"
#include "EditorCore/Services/Selection/SelectionService.h"
#include "EditorCore/Services/ShellCommandService.h"
#include "EditorCore/Services/Selection/SceneSelectionSynchronizer.h"
#include "Panels/Subsystems/AssetBrowserSubsystem.h"
#include "Panels/Subsystems/InspectorSubsystem.h"
#include "Panels/Subsystems/SceneHierarchySubsystem.h"
#include "Panels/Subsystems/ViewportSubsystem.h"

EditorHost::EditorHost(EditorRuntime& runtime):
m_EditorRuntime(runtime)
{
    m_Services = MakeUnique<TEditorServiceContainer>();
    m_ToolService = TUniquePtr<ToolService>(new ToolService());
    m_PanelContainer = TUniquePtr<TPanelContainer>(new TPanelContainer(*this, runtime, *m_ToolService));
    m_DialogManager = MakeUnique<DialogManager>(*this, runtime);

    RegisterCoreSubsystems();
    RegisterCoreServices();
}

void EditorHost::RegisterCoreSubsystems()
{
    // Core subsystems
    m_PanelContainer->RegisterSubsystem<ViewportSubsystem>(*this, m_EditorRuntime, *m_ToolService);
    m_PanelContainer->RegisterSubsystem<SceneHierarchySubsystem>(*this);
    m_PanelContainer->RegisterSubsystem<InspectorSubsystem>(*this);
    m_PanelContainer->RegisterSubsystem<AssetBrowserSubsystem>(*this, m_EditorRuntime);
}

void EditorHost::RegisterCoreServices()
{
    m_Services->Register<SceneQueryService>(m_EditorRuntime);
    m_Services->Register<SelectionService>();
    m_Services->Register<SceneSelectionSynchronizer>(*this, m_EditorRuntime);
    m_Services->Register<HierarchyService>(*this);
    m_Services->Register<ScenePickingService>(*this);
    m_Services->Register<AssetBrowserService>(*this, m_EditorRuntime.GetFile());
    m_Services->Register<EditTimelineService>(*this);
    m_EditorRuntime.SetEditSink(&GetService<EditTimelineService>());
    m_Services->Register<ShellCommandService>(*this);
    m_Services->Register<HotkeyService>(*this, m_EditorRuntime);
}

void EditorHost::Tick(float deltaTime)
{
    m_Services->Tick(deltaTime);
    m_PanelContainer->Tick(deltaTime);
}

void EditorHost::Shutdown()
{
    m_Services->Shutdown();
}

void EditorHost::RegisterShellCommandsForServices()
{
    m_Services->RegisterShellCommand(GetService<ShellCommandService>());
}