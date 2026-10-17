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

    const char* GetFontRegistrationStatusName(Silverware::EFontRegistrationStatus status)
    {
        using Silverware::EFontRegistrationStatus;

        switch (status)
        {
        case EFontRegistrationStatus::Registered: return "Registered";
        case EFontRegistrationStatus::AlreadyRegistered: return "AlreadyRegistered";
        case EFontRegistrationStatus::InvalidResourcePath: return "InvalidResourcePath";
        case EFontRegistrationStatus::InvalidOptions: return "InvalidOptions";
        case EFontRegistrationStatus::UnsupportedFileType: return "UnsupportedFileType";
        case EFontRegistrationStatus::NotFound: return "NotFound";
        case EFontRegistrationStatus::InvalidFontData: return "InvalidFontData";
        case EFontRegistrationStatus::ResourceError: return "ResourceError";
        }

        return "Unknown";
    }
}

SilverwareBackend::~SilverwareBackend()
{
    Shutdown();
}

bool SilverwareBackend::Initialize(EditorRuntime& editorRuntime, IPlatformWindow& window,
    std::span<const Silverware::FResourcePath> registeredFonts)
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

    // Register engine-owned editor fonts.
    // This does not scan the developer's working project assets.
    m_FontRegistry = Silverware::MakeUnique<Silverware::FontRegistry>(*m_ResourceProvider);

    Silverware::FFontDirectoryOptions fontOptions;
    fontOptions.recursive = true;

    const auto directoryReport = m_FontRegistry->RegisterFontDirectory(
        "engine:/Editor/Fonts", fontOptions);

    if (!directoryReport.Completed())
    {
        std::cerr << "[SilverwareBackend]: Failed to enumerate engine editor fonts.\n";
    }

    for (const auto& diagnostic : directoryReport.diagnostics)
    {
        std::cerr << "[SilverwareBackend]: Rejected font "
                  << diagnostic.path.ToString()
                  << " (" << GetFontRegistrationStatusName(diagnostic.status) << ").\n";
    }

    // Import any additional fonts already registered by RedleafEngine.
    const auto fontReport = m_FontRegistry->RegisterFonts(registeredFonts);

    for (const auto& diagnostic : fontReport.diagnostics)
    {
        std::cerr << "[SilverwareBackend]: Rejected font "
                  << diagnostic.path.ToString()
                  << " (" << GetFontRegistrationStatusName(diagnostic.status) << ").\n";
    }

   if (m_FontRegistry->IsEmpty())
        std::cerr << "[SilverwareBackend]: No fonts from the supplied inventory passed validation; continuing without registered fonts.\n";

    if (!InitializeRuntime(window))
    {
        Shutdown();
        return false;
    }

    return true;
}

bool SilverwareBackend::Initialize(EditorRuntime& editorRuntime, IPlatformWindow& window)
{
    return Initialize(editorRuntime, window, std::span<const Silverware::FResourcePath>{});
}

bool SilverwareBackend::MountResources(EditorRuntime& editorRuntime)
{
    m_ResourceProvider = MakeUnique<Silverware::FLayeredResourceProvider>();

    const std::filesystem::path silverwareResourceRoot(
        UPath::Join(UFileSystem::GetEngineRoot().string(), "Frameworks", "Silverware", "Resources"));

    std::error_code filesystemError;
    if (!std::filesystem::is_directory(silverwareResourceRoot, filesystemError))
    {
        std::cerr << "[SilverwareBackend]: Silverware resource directory does not exist or is inaccessible: "
                  << silverwareResourceRoot.string();
        if (filesystemError)
            std::cerr << " (" << filesystemError.message() << ")";
        std::cerr << '\n';
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
    if (!m_RenderingContext || !m_ResourceProvider || !m_FontRegistry)
    {
        std::cerr << "[SilverwareBackend]: Cannot initialize RmlUi before resources and font registry are ready.\n";
        return false;
    }

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
    if (!m_RmlUiRuntime->Initialize(*m_RenderingContext, *m_ResourceProvider, *m_FontRegistry, config, width, height))
    {
        std::cerr << "[SilverwareBackend]: Failed to initialize RmlUi runtime.\n";
        return false;
    }

    return true;
}

bool SilverwareBackend::Synchronize(Silverware::CoreWidget& root, Silverware::EBackendSyncMode mode)
{
    if (!m_RmlUiRuntime || !m_RmlUiRuntime->IsInitialized())
        return false;

    return m_RmlUiRuntime->Synchronize(root, mode);
}

void SilverwareBackend::Render(IPlatformWindow& window)
{
    if (!m_RmlUiRuntime || !m_RmlUiRuntime->IsInitialized())
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
    // RmlUi retains interfaces and document/backend references until shutdown completes.
    if (m_RmlUiRuntime)
    {
        m_RmlUiRuntime->Shutdown();
        m_RmlUiRuntime.reset();
    }

    // FontRegistry borrows the provider, so destroy it before the provider.
    m_FontRegistry.reset();
    m_ResourceProvider.reset();
    m_RenderingContext.reset();
}

bool SilverwareBackend::IsInitialized() const noexcept
{
    return m_RmlUiRuntime && m_RmlUiRuntime->IsInitialized();
}
