#include "PanelContainer.h"
#include "Subsystems/ViewportSubsystem.h"

PanelContainer::PanelContainer(EditorHost& host, EditorRuntime& runtime, ToolService& tools)
    : m_Host(host)
    , m_Runtime(runtime)
    , m_Tools(tools)
{
    m_Viewport = MakeUnique<ViewportSubsystem>(m_Host, m_Runtime, m_Tools);
    //m_Hierarchy = MakeUnique<HierarchySubsystem>(m_Host, m_Runtime, m_Tools);
}

void PanelContainer::Tick(float dt)
{
    m_Viewport->Tick(dt);
    //m_Hierarchy->Tick(dt);
}
