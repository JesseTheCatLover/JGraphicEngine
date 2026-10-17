// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareEditorApp.h"

#include "EditorRuntime.h"
#include "EditorCore/EditorHost.h"
#include "Rendering/IPlatformWindow.h"

#include "Silverware/Backend/IWidgetBackend.h"
#include "Silverware/Backend/IRenderingContext.h"
#include "Silverware/Backends/RmlUi/RmlUiRuntime.h"
#include "Silverware/Core/ContainerWidget.h"
#include "Silverware/Core/Resources/FDirectoryResource.h"
#include "Silverware/Core/Resources/LayeredResourceProvider.h"
#include "Silverware/Core/Resources/ResourcePath.h"

#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

#include <iostream>

#include "Silverware/Core/Resources/FDirectoryResource.h"

SilverwareEditorApp::SilverwareEditorApp()
{
}

SilverwareEditorApp::~SilverwareEditorApp()
{
    Shutdown();
}

void SilverwareEditorApp::Shutdown()
{
    // RmlUi must shut down before its resources and rendering context.
    if (m_RmlUiRuntime)
    {
        m_RmlUiRuntime->Shutdown();
        m_RmlUiRuntime.reset();
    }

    m_RootWidget.reset();

    // The resource sources depend on host-owned infrastructure.
    m_ResourceProvider.reset();

    if (m_EditorHost)
    {
        m_EditorHost->Shutdown();
        m_EditorHost.reset();
    }

    m_RenderingContext.reset();
    m_Window = nullptr;
    m_EditorRuntime.reset();
}

void SilverwareEditorApp::OnProjectInitialized(
    IPlatformWindow* window,
    ProjectContext&)
{
    if (!window)
    {
        std::cerr
            << "[SilverwareEditorApp]: Window is null.\n";
        return;
    }

    Shutdown();

    m_Window = window;

    m_EditorRuntime = TUniquePtr<EditorRuntime>(new EditorRuntime());

    m_EditorHost = TUniquePtr<EditorHost>(new EditorHost(*m_EditorRuntime));

    m_RenderingContext = m_EditorRuntime->CreateSilverwareRenderingContext();

    if (!m_RenderingContext)
    {
        std::cerr << "[SilverwareEditorApp]: Failed to create rendering context.\n";

        Shutdown();
        return;
    }

    m_ResourceProvider =
        MakeUnique<Silverware::FLayeredResourceProvider>();

    // Silverware's own packaged/default resources.
    const std::string frameworkResourcesRoot = UPath::Join(
        UFileSystem::GetEngineRoot().string(),
        "Frameworks",
        "Silverware",
        "Resources");

    Silverware::TSharedPtr<Silverware::IResourceSource>
        frameworkSource = MakeShared<Silverware::FDirectoryResourceSource>(frameworkResourcesRoot);

    if (!frameworkSource || !m_ResourceProvider->Mount("silverware", 0, frameworkSource))
    {
        std::cerr << "[SilverwareEditorApp]: Failed to mount Silverware resources: " << frameworkResourcesRoot << '\n';

        Shutdown();
        return;
    }

    // Engine-owned ordinary file resources.
    auto engineSource = m_EditorRuntime->CreateSilverwareResourceSource("/Engine");

    if (!engineSource || !m_ResourceProvider->Mount("engine", 0, engineSource))
    {
        std::cerr << "[SilverwareEditorApp]: Failed to mount engine resources.\n";

        Shutdown();
        return;
    }

    // Project resources are optional if no project mount exists.
    auto projectSource = m_EditorRuntime->CreateSilverwareResourceSource("/Project");

    if (projectSource)
    {
        if (!m_ResourceProvider->Mount("project", 0, projectSource))
        {
            std::cerr << "[SilverwareEditorApp]: Failed to mount project resources.\n";

            Shutdown();
            return;
        }
    }

    int width = 0;
    int height = 0;
    window->GetFramebufferSize(width, height);

    Silverware::Backends::RmlUi::FRmlUiRuntimeConfig runtimeConfig;

    if (!Silverware::FResourcePath::TryParse(
            "silverware:/Backends/RmlUi/Documents/SilverwareRoot.rml",
            runtimeConfig.rootDocument))
    {
        std::cerr << "[SilverwareEditorApp]: Invalid root document resource path.\n";

        Shutdown();
        return;
    }

    m_RmlUiRuntime = MakeUnique<Silverware::Backends::RmlUi::RmlUiRuntime>();

    if (!m_RmlUiRuntime->Initialize(
            *m_RenderingContext,
            *m_ResourceProvider,
            runtimeConfig,
            width,
            height))
    {
        std::cerr << "[SilverwareEditorApp]: Failed to initialize RmlUi.\n";

        Shutdown();
        return;
    }

    // Create the root widget tree. Actual editor widgets will be added here.
    m_RootWidget = MakeUnique<Silverware::ContainerWidget>();

    auto rootLayout = m_RootWidget->GetLayout();
    rootLayout.width = Silverware::FLayoutLength::Percent(100.0f);
    rootLayout.height = Silverware::FLayoutLength::Percent(100.0f);
    m_RootWidget->SetLayout(rootLayout);

    if (!m_RmlUiRuntime->Synchronize(*m_RootWidget, Silverware::EBackendSyncMode::Full))
    {
        std::cerr << "[SilverwareEditorApp]: Initial widget synchronization failed.\n";

        Shutdown();
        return;
    }
}

void SilverwareEditorApp::OnSceneLoaded(const std::string&)
{
}

void SilverwareEditorApp::OnTick(float deltaTime)
{
    if (m_EditorHost)
        m_EditorHost->Tick(deltaTime);

    if (!m_RmlUiRuntime || !m_RootWidget)
        return;

    if (!m_RmlUiRuntime->Synchronize(*m_RootWidget, Silverware::EBackendSyncMode::Incremental))
    {
        std::cerr << "[SilverwareEditorApp]: Incremental synchronization failed.\n";
    }
}

void SilverwareEditorApp::OnRenderOverlay(float)
{
    if (!m_Window || !m_RmlUiRuntime)
        return;

    int width = 0;
    int height = 0;
    m_Window->GetFramebufferSize(width, height);

    if (width <= 0 || height <= 0)
        return;

    m_RmlUiRuntime->Resize(width, height);
    m_RmlUiRuntime->Update();
    m_RmlUiRuntime->Render();
}