//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "ViewportSubsystem.h"
#include "ToolService.h"

ViewportSubsystem::ViewportSubsystem(EditorHost &host, EditorRuntime &runtime, ToolService &tools)
: m_Host(host)
      , m_Runtime(runtime)
      , m_Tools(tools)
      , m_Channel([this](PanelID id)
          {
              return std::make_unique<ViewportController>(id, m_Host, m_Runtime, m_Tools);
          })
{}