// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareBackend.h"

#include "EditorRuntime.h"
#include "Rendering/IPlatformWindow.h"

#include "Silverware/Core/Resources/FDirectoryResource.h"
#include "Silverware/Core/Resources/ResourcePath.h"

#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

#include <filesystem>
#include <iostream>

namespace
{
    constexpr char kSilverwareRootDocument[] =
        "silverware:/Backends/RmlUi/Documents/SilverwareRoot.rml";
}

SilverwareBackend::~SilverwareBackend()
{
    Shutdown();
}

bool SilverwareBackend::Initialize( EditorRuntime& editorRuntime, IPlatformWindow& window)
{
    Shutdown();

    m_RenderingContext = editorRuntime.CreateSilverwareRenderingContext();

    if (!m_RenderingContext)
    {
        std::cerr << "[SilverwareBackend]: Failed to create rendering context.\n";
        Shutdown();
        return false;
    }

    if (!MountResources(editorRuntime))
    {
        Shutdown();
        return false;
    }

    if (!InitializeRuntime(window))
    {
        Shutdown();
        return false;
    }

    return true;
}

bool SilverwareBackend::MountResources(EditorRuntime& editorRuntime)
{
    m_ResourceProvider = MakeUnique<Silverware::FLayeredResourceProvider>();

    // Silverware's own resources: RML documents, stylesheets, fonts, etc.
    const std::filesystem::path silverwareResourceRoot(
        UPath::Join(
            UFileSystem::GetEngineRoot().string(),
            "Frameworks",
            "Silverware",
            "Resources"));

    if (!std::filesystem::is_directory(silverwareResourceRoot))
    {
        std::cerr << "[SilverwareBackend]: Silverware resource directory does not "
        << "exist: " << silverwareResourceRoot.string() << '\n';
        return false;
    }

    auto silverwareSource = MakeShared<Silverware::FDirectoryResource>(silverwareResourceRoot);

    if (!m_ResourceProvider->Mount("silverware", 0, silverwareSource))
    {
        std::cerr << "[SilverwareBackend]: Failed to mount Silverware resources.\n";
        return false;
    }

    // Engine assets are exposed through RedleafEngine's existing virtual paths.
    auto engineSource = editorRuntime.CreateSilverwareResourceSource("/Engine");

    if (!engineSource || !m_ResourceProvider->Mount("engine", 0, engineSource))
    {
        std::cerr << "[SilverwareBackend]: Failed to mount engine resources.\n";
        return false;
    }

    // Projects may not always have a mounted project resource root.
    auto projectSource = editorRuntime.CreateSilverwareResourceSource("/Project");

    if (projectSource && !m_ResourceProvider->Mount("project", 0, projectSource))
    {
        std::cerr << "[SilverwareBackend]: Failed to mount project resources.\n";
        return false;
    }

    return true;
}

bool SilverwareBackend::InitializeRuntime(IPlatformWindow& window)
{
    int width = 0;
    int height = 0;
    window.GetFramebufferSize(width, height);

    // Allow initialization if the window is temporarily minimized.
    if (width <= 0 || height <= 0)
    {
        width = 1;
        height = 1;
    }

    Silverware::Backends::RmlUi::FRmlUiRuntimeConfig config;

    if (!Silverware::FResourcePath::TryParse(kSilverwareRootDocument, config.rootDocument))
    {
        std::cerr << "[SilverwareBackend]: Invalid root document resource path.\n";
        return false;
    }

    m_RmlUiRuntime = MakeUnique<Silverware::Backends::RmlUi::RmlUiRuntime>();

    if (!m_RmlUiRuntime->Initialize(*m_RenderingContext, *m_ResourceProvider, config, width, height))
    {
        std::cerr << "[SilverwareBackend]: Failed to initialize RmlUi runtime.\n";
        return false;
    }

    return true;
}

bool SilverwareBackend::Synchronize(Silverware::CoreWidget& root, Silverware::EBackendSyncMode mode)
{
    if (!m_RmlUiRuntime)
        return false;

    return m_RmlUiRuntime->Synchronize(root, mode);
}

void SilverwareBackend::Render(IPlatformWindow& window)
{
    if (!m_RmlUiRuntime)
        return;

    int width = 0;
    int height = 0;
    window.GetFramebufferSize(width, height);

    if (width <= 0 || height <= 0)
        return;

    m_RmlUiRuntime->Resize(width, height);
    m_RmlUiRuntime->Update();
    m_RmlUiRuntime->Render();
}

void SilverwareBackend::Shutdown()
{
    // RmlUiRuntime is responsible for tearing down its widget synchronizer
    // and backend before shutting RmlUi itself down.
    if (m_RmlUiRuntime)
    {
        m_RmlUiRuntime->Shutdown();
        m_RmlUiRuntime.reset();
    }

    // Resource sources created by EditorRuntime may borrow its infrastructure.
    // Destroy all sources before destroying EditorRuntime.
    m_ResourceProvider.reset();
    m_RenderingContext.reset();
}