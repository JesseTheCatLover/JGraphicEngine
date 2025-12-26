//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "EditorRuntime.h"
#include "Core/Memory/SmartPointers.h"

class ToolService;
class PanelContainer;

class EditorHost
{
    friend class EditorApp;
private:
    explicit EditorHost(EditorRuntime& runtime);

    EditorRuntime& m_EditorRuntime;
    TUniquePtr<PanelContainer> m_PanelContainer;
    TUniquePtr<ToolService> m_ToolService;

public:
    // Called every frame from EditorApp::OnTick
    void Tick(float deltaTime);

    // Getters
    [[nodiscard]] PanelContainer* GetPanelContainer() const { return m_PanelContainer.get(); }
    [[nodiscard]] ToolService* GetToolService() const { return m_ToolService.get(); }
};