//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "EditorRuntime.h"
#include "TEditorServiceContainer.h"
#include "TPanelContainer.h"
#include "Core/Memory/SmartPointers.h"
#include "Subsystems/ViewportSubsystem.h"

class EditorLayoutModel;
class ToolService;

class EditorHost
{
    friend class EditorApp;
private:
    explicit EditorHost(EditorRuntime& runtime);

    EditorRuntime& m_EditorRuntime;
    TUniquePtr<ToolService> m_ToolService;
    TUniquePtr<TPanelContainer> m_PanelContainer;
    TUniquePtr<TEditorServiceContainer> m_Services;

    void RegisterCoreSubsystems();
    void RegisterCoreServices();

public:
    // Called every frame from EditorApp::OnTick
    void Tick(float deltaTime);

    void RegisterShellCommandsForServices();

    // Getters
    [[nodiscard]] TPanelContainer* GetPanelContainer() const { return m_PanelContainer.get(); }
    [[nodiscard]] ToolService* GetToolService() const { return m_ToolService.get(); }

    template<typename TSubsystem>
    TSubsystem& GetSubsystem()
    {
        return m_PanelContainer->GetSubsystem<TSubsystem>();
    }

    template<typename T>
    T& GetService() { return m_Services->Get<T>(); }

    EditorRuntime& GetRuntime() { return m_EditorRuntime; }

};