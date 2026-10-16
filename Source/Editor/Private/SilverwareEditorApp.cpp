// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareEditorApp.h"

#include "EditorRuntime.h"
#include "EditorCore/EditorHost.h"
#include "Core/Project/ProjectContext.h"
#include "Silverware/Backends/RmlUi/RmlUiRuntime.h"
#include "Silverware/Backend/IRenderingContext.h"
#include "Rendering/IPlatformWindow.h"

#include <iostream>

#include "Silverware/Backend/IWidgetBackend.h"
#include "Silverware/Core/ContainerWidget.h"
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

    m_RootWidget =
    TUniquePtr<Silverware::ContainerWidget>(new FEditorRootWidget());

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