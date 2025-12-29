//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "IPanelSubsystem.h"
#include "TPanelSubsystem.h"

#include "Controllers/SceneHierarchyController.h"
#include "Controllers/Inputs/FHierarchyPanelInput.h"
#include "Controllers/Outputs/FHierarchyOutput.h"

class EditorHost;

class SceneHierarchySubsystem : public IPanelSubsystem
{
    EditorHost& m_Host;

    using Channel = TPanelSubsystem<FHierarchyPanelInput, FHierarchyOutput, SceneHierarchyController>;
    Channel m_Channel;

public:
    explicit SceneHierarchySubsystem(EditorHost& host)
        : m_Host(host)
        , m_Channel([this](PanelID id)
        {
            return std::make_unique<SceneHierarchyController>(id, m_Host);
        })
    {}

    void Tick(float deltaTime) override { m_Channel.Tick(deltaTime); }

    void SubmitInput(const FHierarchyPanelInput& input) { m_Channel.SubmitInput(input); }

    const FHierarchyOutput* GetOutput(const char* panelKey) const
    {
        return m_Channel.GetOutput(panelKey);
    }

    void Destroy(const char* panelKey) { m_Channel.Destroy(panelKey); }
};
