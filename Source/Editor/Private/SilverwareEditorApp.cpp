// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareEditorApp.h"

#include "EditorRuntime.h"
#include "EditorCore/EditorHost.h"
#include "Core/Project/ProjectContext.h"
#include "Silverware/Backends/RmlUi/RmlUiRuntime.h"
#include "Silverware/Backend/IRenderingContext.h"
#include "Rendering/IPlatformWindow.h"

#include <iostream>

#include <array>
#include <span>

#include "Silverware/Core/Resources/IResourceProvider.h"
#include "Silverware/Core/Resources/IResourceStream.h"
#include "Silverware/Core/Resources/ResourcePath.h"

#include "Silverware/Backend/IWidgetBackend.h"
#include "Silverware/Core/ContainerWidget.h"
#include "Silverware/Core/Resources/LayeredResourceProvider.h"
#include "Silverware/UI/TextWidget.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

namespace
{
    class FEditorRootWidget final : public Silverware::ContainerWidget
    {
    public:
        FEditorRootWidget() = default;
    };

bool RunSilverwareResourceSmokeTest(
    Silverware::IResourceProvider& provider)
{
    using namespace Silverware;

    constexpr char kFontURI[] =
        "engine:/Editor/Fonts/FunnelSans.ttf";

    FResourcePath fontPath;

    if (!FResourcePath::TryParse(kFontURI, fontPath))
    {
        std::cerr
            << "[SilverwareResourceTest]: Failed to parse font path.\n";
        return false;
    }

    Silverware::TUniquePtr<IResourceStream> stream;

    if (provider.Open(fontPath, stream) != EResourceOpenResult::Opened ||
        !stream)
    {
        std::cerr
            << "[SilverwareResourceTest]: Failed to open "
            << kFontURI << '\n';
        return false;
    }

    const std::uint64_t size = stream->GetSize();

    if (size < 4)
    {
        std::cerr
            << "[SilverwareResourceTest]: Font resource is unexpectedly small.\n";
        return false;
    }

    // Read the first four bytes.
    std::array<std::byte, 4> header{};
    std::size_t bytesRead = 0;

    if (!stream->Read(std::span<std::byte>(header), bytesRead) ||
        bytesRead != header.size())
    {
        std::cerr
            << "[SilverwareResourceTest]: Initial read failed.\n";
        return false;
    }

    // Seek to the beginning and verify deterministic reading.
    if (!stream->Seek(0))
    {
        std::cerr
            << "[SilverwareResourceTest]: Seek to beginning failed.\n";
        return false;
    }

    std::array<std::byte, 4> headerAgain{};
    bytesRead = 0;

    if (!stream->Read(
            std::span<std::byte>(headerAgain), bytesRead) ||
        bytesRead != headerAgain.size() ||
        header != headerAgain)
    {
        std::cerr
            << "[SilverwareResourceTest]: Read-after-seek failed.\n";
        return false;
    }

    // Seeking to EOF and reading should succeed with zero bytes.
    if (!stream->Seek(size))
    {
        std::cerr
            << "[SilverwareResourceTest]: Seek to EOF failed.\n";
        return false;
    }

    std::array<std::byte, 1> finalByte{};
    bytesRead = 0;

    if (!stream->Read(std::span<std::byte>(finalByte), bytesRead) ||
        bytesRead != 0)
    {
        std::cerr
            << "[SilverwareResourceTest]: EOF behavior is incorrect.\n";
        return false;
    }

    // A missing resource must report NotFound.
    FResourcePath missingPath;

    if (!FResourcePath::TryParse(
            "engine:/__silverware_test__/missing.bin",
            missingPath))
    {
        return false;
    }

    Silverware::TUniquePtr<IResourceStream> missingStream;

    if (provider.Open(missingPath, missingStream) !=
            EResourceOpenResult::NotFound ||
        missingStream)
    {
        std::cerr
            << "[SilverwareResourceTest]: Missing-resource handling failed.\n";
        return false;
    }

    // Verify relative path resolution stays in the same namespace.
    FResourcePath relativePath;

    if (!FResourcePath::TryResolveRelative(
            fontPath,
            "../Textures/AssetBrowser/Icon.jasset",
            relativePath) ||
        relativePath.ToString() !=
            "engine:/Editor/Textures/AssetBrowser/Icon.jasset")
    {
        std::cerr
            << "[SilverwareResourceTest]: Relative path resolution failed.\n";
        return false;
    }

    std::cout
        << "[SilverwareResourceTest]: PASSED\n"
        << "  Resource: " << fontPath.ToString() << '\n'
        << "  Size: " << size << " bytes\n"
        << "  Read/seek: OK\n"
        << "  EOF behavior: OK\n"
        << "  Missing resource: OK\n"
        << "  Relative paths: OK\n";

    return true;
}
}

SilverwareEditorApp::SilverwareEditorApp()
{
}

SilverwareEditorApp::~SilverwareEditorApp()
{
    Shutdown();
}

void SilverwareEditorApp::Shutdown()
{
    if (m_RmlUiRuntime)
    {
        m_RmlUiRuntime->Shutdown();
        m_RmlUiRuntime.reset();
    }

    // The runtime/backend must be destroyed before its widget tree.
    m_RootWidget.reset();
    m_TestTextWidget = nullptr;
    m_TestDynamicTextWidget = nullptr;
    m_SyncTestElapsed = 0.0f;
    m_SyncTestStage = 0;

    // Resource sources borrow host-owned infrastructure.
    // Release them before destroying the host-facing runtime.
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

void SilverwareEditorApp::OnProjectInitialized(IPlatformWindow* window, ProjectContext& projectCtx)
{
    if (!window)
    {
        std::cerr << "[SilverwareEditorApp]: Window is null.\n";
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

    m_ResourceProvider = MakeUnique<Silverware::FLayeredResourceProvider>();

    auto engineSource = m_EditorRuntime->CreateSilverwareResourceSource("/Engine");

    if (!engineSource || !m_ResourceProvider->Mount("engine", 0, engineSource))
    {
        std::cerr << "[SilverwareEditorApp]: Failed to mount engine resources.\n";
        Shutdown();
        return;
    }

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
    else
    {
        std::cerr << "[SilverwareEditorApp]: Project resource mount is unavailable.\n";
    }

    if (!RunSilverwareResourceSmokeTest(*m_ResourceProvider))
    {
        std::cerr
            << "[SilverwareEditorApp]: Resource smoke test failed.\n";
    }

    int width = 0;
    int height = 0;
    window->GetFramebufferSize(width, height);

    const std::string fontPath = UPath::Join(
        UFileSystem::GetEngineRoot().string(),
        "Frameworks",
        "Silverware",
        "ThirdParty",
        "RmlUi",
        "Samples",
        "assets",
        "LatoLatin-Regular.ttf"
    );

    m_RmlUiRuntime = MakeUnique<Silverware::Backends::RmlUi::RmlUiRuntime>();

    if (!m_RmlUiRuntime->Initialize(
            *m_RenderingContext, width, height, fontPath))
    {
        std::cerr << "[SilverwareEditorApp]: Failed to initialize RmlUi.\n";
        Shutdown();
        return;
    }

    m_RootWidget = TUniquePtr<Silverware::ContainerWidget>(new FEditorRootWidget());

    // Root layout
    auto rootLayout = m_RootWidget->GetLayout();
    rootLayout.width = Silverware::FLayoutLength::Percent(100.0f);
    rootLayout.height = Silverware::FLayoutLength::Percent(100.0f);
    m_RootWidget->SetLayout(rootLayout);

    // Container layout
    auto containerLayout = m_RootWidget->GetContainerLayout();
    containerLayout.direction = Silverware::ELayoutDirection::Horizontal;
    containerLayout.gap = Silverware::FLayoutLength::Pixels(30.0f);
    containerLayout.alignItems = Silverware::EAlignment::Start;
    containerLayout.justifyContent = Silverware::EJustification::Center;
    m_RootWidget->SetContainerLayout(containerLayout);

    // Create the text widget
    auto* textWidget = new Silverware::TextWidget("Hello from the Silverware widget tree!");

    if (!m_RootWidget->AddChild(Silverware::TUniquePtr<Silverware::CoreWidget>(textWidget)))
    {
        std::cerr << "[SilverwareEditorApp]: Failed to add initial text widget.\n";
        Shutdown();
        return;
    }

    m_TestTextWidget = textWidget;

    // Text layout
    auto textLayout = m_TestTextWidget->GetLayout();
    textLayout.margin.top = Silverware::FLayoutLength::Pixels(24.0f);
    textLayout.margin.left = Silverware::FLayoutLength::Pixels(24.0f);
    m_TestTextWidget->SetLayout(textLayout);

    m_SyncTestElapsed = 0.0f;
    m_SyncTestStage = 0;

    // Initial synchronization
    if (!m_RmlUiRuntime->Synchronize(
            *m_RootWidget,
            Silverware::EBackendSyncMode::Full))
    {
        std::cerr << "[SilverwareEditorApp]: Initial widget synchronization failed.\n";
        Shutdown();
        return;
    }

    // Silverware backend and UI initialization will be added here.
    (void)projectCtx;
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

    if (m_SyncTestStage < 3)
    {
        m_SyncTestElapsed += deltaTime;

        if (m_SyncTestElapsed >= 2.0f)
        {
            m_SyncTestElapsed = 0.0f;

            switch (m_SyncTestStage)
            {
            case 0:
                if (m_TestTextWidget)
                {
                    m_TestTextWidget->SetText(
                        "Incremental property update succeeded!");

                    std::cout
                        << "[SilverwareEditorApp]: Testing property update.\n";
                }
                break;

            case 1:
            {
                Silverware::TUniquePtr<Silverware::CoreWidget> child(
                    new Silverware::TextWidget(
                        "Runtime child added successfully!"));

                auto* textWidget = static_cast<Silverware::TextWidget*>(child.get());

                if (m_RootWidget->AddChild(
                        Silverware::TakeUniqueOwnership(child)))
                {
                    m_TestDynamicTextWidget = textWidget;
                    auto textLayout = m_TestDynamicTextWidget->GetLayout();
                    textLayout.margin.top = Silverware::FLayoutLength::Pixels(24.0f);
                    textLayout.margin.left = Silverware::FLayoutLength::Pixels(24.0f);
                    m_TestDynamicTextWidget->SetLayout(textLayout);

                    std::cout
                        << "[SilverwareEditorApp]: Testing child addition.\n";
                }
                else
                {
                    std::cerr
                        << "[SilverwareEditorApp]: Failed to add test child.\n";
                }

                break;
            }

            case 2:
                if (m_TestDynamicTextWidget)
                {
                    auto removed =
                        m_RootWidget->RemoveChild(*m_TestDynamicTextWidget);

                    if (removed)
                    {
                        m_TestDynamicTextWidget = nullptr;

                        std::cout
                            << "[SilverwareEditorApp]: Testing child removal.\n";
                    }
                    else
                    {
                        std::cerr
                            << "[SilverwareEditorApp]: Failed to remove test child.\n";
                    }
                }
                break;

            default:
                break;
            }

            ++m_SyncTestStage;
        }
    }

    if (!m_RmlUiRuntime->Synchronize(
            *m_RootWidget,
            Silverware::EBackendSyncMode::Incremental))
    {
        std::cerr
            << "[SilverwareEditorApp]: Incremental synchronization failed.\n";
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