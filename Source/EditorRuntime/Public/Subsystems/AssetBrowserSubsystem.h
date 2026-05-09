// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "IPanelSubsystem.h"
#include "TPanelSubsystem.h"

#include "Controllers/AssetBrowserController.h"
#include "Controllers/Inputs/FAssetBrowserPanelInput.h"
#include "Controllers/Outputs/FAssetBrowserOutput.h"

class EditorHost;
class EditorRuntime;

class AssetBrowserSubsystem : public IPanelSubsystem
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;

    using Channel = TPanelSubsystem<FAssetBrowserPanelInput, FAssetBrowserOutput, AssetBrowserController>;
    Channel m_Channel;

public:
    AssetBrowserSubsystem(EditorHost& host, EditorRuntime& runtime)
    : m_Host(host)
    , m_Runtime(runtime)
    , m_Channel([this](PanelID id)
    {
        return std::make_unique<AssetBrowserController>(id, m_Host, m_Runtime);
    })
    {}

    void Tick(float deltaTime) override { m_Channel.Tick(deltaTime); }

    void SubmitInput(const FAssetBrowserPanelInput& input) { m_Channel.SubmitInput(input); }

    const FAssetBrowserOutput* GetOutput(const char* panelKey) const
    {
        return m_Channel.GetOutput(panelKey);
    }

    void Destroy(const char* panelKey) { m_Channel.Destroy(panelKey); }
};
