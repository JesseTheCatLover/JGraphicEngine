//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "IPanelSubsystem.h"
#include "Panels//TPanelSubsystem.h"

#include "Panels/Controllers/Inputs/FInspectorPanelInput.h"
#include "Panels/Controllers/Outputs/FInspectorOutput.h"
#include "Panels/Controllers/InspectorController.h"

class EditorHost;

class InspectorSubsystem : public IPanelSubsystem
{
    EditorHost& m_Host;

    using Channel = TPanelSubsystem<FInspectorPanelInput, FInspectorOutput, InspectorController>;
    Channel m_Channel;

public:
    explicit InspectorSubsystem(EditorHost& host)
        : m_Host(host)
        , m_Channel([this](PanelID id)
        {
            return MakeUnique<InspectorController>(id, m_Host);
        })
    {}

    void Tick(float deltaTime) override { m_Channel.Tick(deltaTime); }

    void SubmitInput(const FInspectorPanelInput& input) { m_Channel.SubmitInput(input); }

    const FInspectorOutput* GetOutput(const char* panelKey) const
    {
        return m_Channel.GetOutput(panelKey);
    }

    void Destroy(const char* panelKey) { m_Channel.Destroy(panelKey); }
};