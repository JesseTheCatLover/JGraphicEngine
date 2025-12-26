//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "PanelRegistry.h"
#include "Utilities/UDynamicID.h"
#include "TPanelSubsystem.h"

#include "Controllers/ViewportController.h"
#include "Controllers/Inputs/FViewportPanelInput.h"
#include "Controllers/Outputs/FViewportOutput.h"

class EditorHost;
class EditorRuntime;
class ToolService;

class ViewportSubsystem
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;
    ToolService& m_Tools;

    using Channel = TPanelSubsystem<FViewportPanelInput, FViewportOutput, ViewportController>;
    Channel m_Channel;

public:
    ViewportSubsystem(EditorHost& host, EditorRuntime& runtime, ToolService& tools);

    void Tick(float dt) { m_Channel.Tick(dt); }

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
