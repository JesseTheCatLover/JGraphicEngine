#pragma once

#include "Core/Memory/SmartPointers.h"

class EditorHost;
class EditorRuntime;
class ToolService;

class ViewportSubsystem;
class HierarchySubsystem;

class PanelContainer
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;
    ToolService& m_Tools;

    TUniquePtr<ViewportSubsystem> m_Viewport;
    //TUniquePtr<HierarchySubsystem> m_Hierarchy;

public:
    PanelContainer(EditorHost& host, EditorRuntime& runtime, ToolService& tools);

    void Tick(float dt);

    ViewportSubsystem& GetViewport() { return *m_Viewport; }
    //HierarchySubsystem& GetHierarchy() { return *m_Hierarchy; }
};
