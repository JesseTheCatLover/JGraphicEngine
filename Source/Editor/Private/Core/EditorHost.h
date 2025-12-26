//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "EditorRuntime.h"
#include "Core/Memory/SmartPointers.h"
#include "Subsystems/ViewportSubsystem.h"

class ToolService;
class TPanelContainer;

class EditorHost
{
    friend class EditorApp;
private:
    explicit EditorHost(EditorRuntime& runtime);

    EditorRuntime& m_EditorRuntime;
    TUniquePtr<ToolService> m_ToolService;
    TUniquePtr<TPanelContainer> m_PanelContainer;

    void RegisterCoreSubsystems();

public:
    // Called every frame from EditorApp::OnTick
    void Tick(float deltaTime);

    // Getters
    [[nodiscard]] TPanelContainer* GetPanelContainer() const { return m_PanelContainer.get(); }
    [[nodiscard]] ToolService* GetToolService() const { return m_ToolService.get(); }

    [[nodiscard]] ViewportSubsystem& GetViewportSubsystem() const;
};