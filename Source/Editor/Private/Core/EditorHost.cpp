//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EditorHost.h"

#include "TPanelContainer.h"
#include "ToolService.h"
#include "Services/HierarchyService.h"
#include "Services/PickingService.h"
#include "Services/SceneQueryService.h"
#include "Services/SelectionService.h"
#include "Subsystems/SceneHierarchySubsystem.h"
#include "Subsystems/ViewportSubsystem.h"

EditorHost::EditorHost(EditorRuntime& runtime):
m_EditorRuntime(runtime)
{
    m_Services = MakeUnique<TEditorServiceContainer>();
    m_ToolService = TUniquePtr<ToolService>(new ToolService());
    m_PanelContainer = TUniquePtr<TPanelContainer>(new TPanelContainer(*this, runtime, *m_ToolService));

    RegisterCoreSubsystems();
    RegisterCoreServices();
}

void EditorHost::RegisterCoreSubsystems()
{
    // Core subsystems
    m_PanelContainer->RegisterSubsystem<ViewportSubsystem>(*this, m_EditorRuntime, *m_ToolService);
    m_PanelContainer->RegisterSubsystem<SceneHierarchySubsystem>(*this);
}

void EditorHost::RegisterCoreServices()
{
     m_Services->Register<SceneQueryService>(m_EditorRuntime);
     m_Services->Register<SelectionService>(m_EditorRuntime);
     m_Services->Register<HierarchyService>(*this);
     m_Services->Register<PickingService>(*this);
}

void EditorHost::Tick(float deltaTime)
{
    m_Services->Tick(deltaTime);
    m_PanelContainer->Tick(deltaTime);
}