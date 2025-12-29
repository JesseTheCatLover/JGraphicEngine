//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "IPanelSubsystem.h"

#include "TPanelSubsystem.h"
#include "ToolService.h"
#include "Controllers/ViewportController.h"
#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Controllers/Outputs/FViewportOutput.h"

class EditorHost;
class EditorRuntime;
class ToolService;

class ViewportSubsystem : public IPanelSubsystem
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;
    ToolService& m_Tools;

    using Channel = TPanelSubsystem<FViewportPanelInput, FViewportOutput, ViewportController>;
    Channel m_Channel;

public:
    ViewportSubsystem(EditorHost &host, EditorRuntime &runtime, ToolService &tools)
    : m_Host(host)
          , m_Runtime(runtime)
          , m_Tools(tools)
          , m_Channel([this](PanelID id)
              {
                  return MakeUnique<ViewportController>(id, m_Host, m_Runtime, m_Tools);
              })
        {}

    void Tick(float deltaTime) override { m_Channel.Tick(deltaTime); }

    void SubmitInput(const FViewportPanelInput& input) { m_Channel.SubmitInput(input); }

    const FViewportOutput* GetOutput(const char* panelKey) const
    {
        return m_Channel.GetOutput(panelKey);
    }

    void Destroy(const char* panelKey)
    {
        m_Channel.Destroy(panelKey);
    }
};
