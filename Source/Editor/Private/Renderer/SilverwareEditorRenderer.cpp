// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareEditorRenderer.h"

#include "EditorCore/EditorHost.h"
#include "EditorRuntime.h"
#include "Backends/SilverwareBackend.h"

#include "Panels/Controllers/Inputs/FViewportPanelInput.h"
#include "Panels/Controllers/Outputs/FViewportOutput.h"
#include "Panels/Subsystems/ViewportSubsystem.h"

#include "Silverware/Backend/IRenderingContext.h"
#include "Silverware/Core/ContainerWidget.h"
#include "Silverware/Core/CoreWidget.h"
#include "Silverware/Core/Styling/ContainerStyle.h"
#include "Silverware/Core/Styling/ImageStyle.h"
#include "Silverware/Core/Styling/TextStyle.h"
#include "Silverware/Core/WidgetLayout.h"
#include "Silverware/UI/ImageWidget.h"
#include "Silverware/UI/TextWidget.h"

#include <cstdint>
#include <iostream>

#include "Silverware/Backend/IWidgetBackend.h"

namespace
{
    constexpr char kViewportPanelKey[] = "Viewport:0";

    // Temporary fixed resolution for the first viewport-rendering test.
    constexpr float kViewportRenderWidth = 1280.0f;
    constexpr float kViewportRenderHeight = 720.0f;

    constexpr float kViewportImageHeight = 480.0f;

    void SubmitViewportInput(Silverware::Backends::RmlUi::RmlUiRuntime* unused)
    {
        (void)unused;
    }

    void SubmitViewportInput(ViewportSubsystem& viewportSubsystem)
    {
        FViewportPanelInput input{};
        input.panelKey = kViewportPanelKey;
        input.width = kViewportRenderWidth;
        input.height = kViewportRenderHeight;
        input.bAppFocused = true;
        input.bHidden = false;

        // Rendering-only smoke test: no viewport interaction is submitted yet.
        viewportSubsystem.SubmitInput(input);
    }

    bool AddEditorSection(
        Silverware::ContainerWidget& root,
        const char* title,
        float widthPercent,
        Silverware::ImageWidget** outImage = nullptr)
    {
        using namespace Silverware;

        if (outImage)
            *outImage = nullptr;

        auto section = Silverware::MakeUnique<ContainerWidget>();

        auto sectionLayout = section->GetLayout();
        sectionLayout.width = FLayoutLength::Percent(widthPercent);
        sectionLayout.height = FLayoutLength::Percent(100.0f);
        section->SetLayout(sectionLayout);

        FContainerLayout sectionContainerLayout;
        sectionContainerLayout.direction = ELayoutDirection::Vertical;
        sectionContainerLayout.gap = FLayoutLength::Pixels(8.0f);
        sectionContainerLayout.alignItems = EAlignment::Start;
        sectionContainerLayout.justifyContent = EJustification::Start;
        section->SetContainerLayout(sectionContainerLayout);

        FContainerStyle sectionStyle;
        sectionStyle.backgroundColor = FColor{0.12f, 0.12f, 0.14f, 1.0f};
        sectionStyle.borderColor = FColor{0.25f, 0.25f, 0.28f, 1.0f};
        sectionStyle.borderWidth = 1.0f;
        sectionStyle.borderStyle = EBorderStyle::Solid;
        sectionStyle.cornerRadius = 8.0f;
        sectionStyle.opacity = 1.0f;
        section->SetContainerStyle(sectionStyle);

        auto label = Silverware::MakeUnique<TextWidget>(title);

        auto labelLayout = label->GetLayout();
        labelLayout.margin.top = FLayoutLength::Pixels(8.0f);
        labelLayout.margin.left = FLayoutLength::Pixels(8.0f);
        label->SetLayout(labelLayout);

        FTextStyle labelStyle;
        labelStyle.fontFamily = "LatoLatin";
        labelStyle.fontSize = 30.0f;
        labelStyle.color = FColor{1.0f, 1.0f, 1.0f, 1.0f};
        labelStyle.fontWeight = EFontWeight::Normal;
        label->SetTextStyle(labelStyle);

        if (!section->AddChild(Silverware::TUniquePtr<CoreWidget>(label.release())))
            return false;

        ImageWidget* imageWidget = nullptr;

        if (outImage)
        {
            auto image = Silverware::MakeUnique<ImageWidget>();

            auto imageLayout = image->GetLayout();
            imageLayout.width = FLayoutLength::Percent(100.0f);
            imageLayout.height = FLayoutLength::Pixels(kViewportImageHeight);
            image->SetLayout(imageLayout);

            FImageStyle imageStyle;
            imageStyle.fit = EImageFit::Contain;
            imageStyle.opacity = 1.0f;
            image->SetImageStyle(imageStyle);

            imageWidget = image.get();

            if (!section->AddChild(Silverware::TUniquePtr<CoreWidget>(image.release())))
                return false;
        }

        if (!root.AddChild(Silverware::TUniquePtr<CoreWidget>(section.release())))
            return false;

        if (outImage)
            *outImage = imageWidget;

        return true;
    }
}

SilverwareEditorRenderer::SilverwareEditorRenderer()
{
}

SilverwareEditorRenderer::~SilverwareEditorRenderer()
{
    Shutdown();
}

bool SilverwareEditorRenderer::Initialize(
    EditorHost& editorHost,
    EditorRuntime& editorRuntime,
    SilverwareBackend& backend)
{
    Shutdown();

    if (!backend.IsInitialized())
    {
        std::cerr << "[SilverwareRenderer]: Backend is not initialized.\n";
        return false;
    }

    m_EditorHost = &editorHost;
    m_EditorRuntime = &editorRuntime;
    m_Backend = &backend;

    m_RootWidget = Silverware::MakeUnique<Silverware::ContainerWidget>();

    auto rootLayout = m_RootWidget->GetLayout();
    rootLayout.width = Silverware::FLayoutLength::Percent(100.0f);
    rootLayout.height = Silverware::FLayoutLength::Percent(100.0f);
    m_RootWidget->SetLayout(rootLayout);

    Silverware::FContainerLayout rootContainerLayout;
    rootContainerLayout.direction = Silverware::ELayoutDirection::Horizontal;
    rootContainerLayout.gap = Silverware::FLayoutLength::Pixels(8.0f);
    rootContainerLayout.alignItems = Silverware::EAlignment::Start;
    rootContainerLayout.justifyContent = Silverware::EJustification::Start;
    m_RootWidget->SetContainerLayout(rootContainerLayout);

    if (!BuildEditorLayout())
    {
        std::cerr << "[SilverwareRenderer]: Failed to build editor layout.\n";
        Shutdown();
        return false;
    }

    // Prime the viewport subsystem so its controller can create a render target
    // on the next EditorHost tick.
    SubmitViewportInput(editorHost.GetSubsystem<ViewportSubsystem>());

    if (!m_Backend->Synchronize(
            *m_RootWidget,
            Silverware::EBackendSyncMode::Full))
    {
        std::cerr << "[SilverwareRenderer]: Initial synchronization failed.\n";
        Shutdown();
        return false;
    }

    return true;
}

bool SilverwareEditorRenderer::BuildEditorLayout()
{
    if (!m_RootWidget)
        return false;

    return
        AddEditorSection(*m_RootWidget, "Scene Hierarchy", 20.0f) &&
        AddEditorSection(*m_RootWidget, "Viewport", 55.0f, &m_ViewportImage) &&
        AddEditorSection(*m_RootWidget, "Inspector", 20.0f);
}

void SilverwareEditorRenderer::Tick(float deltaTime)
{
    (void)deltaTime;

    if (!m_Backend || !m_RootWidget || !m_EditorHost || !m_ViewportImage)
        return;

    auto& viewportSubsystem = m_EditorHost->GetSubsystem<ViewportSubsystem>();

    // The output is produced by ViewportSubsystem during EditorHost::Tick().
    // Read its current texture before submitting the next input snapshot.
    const FViewportOutput* output = viewportSubsystem.GetOutput(kViewportPanelKey);

    if (output &&
        output->textureHandle.IsValid() &&
        output->width > 0 &&
        output->height > 0)
    {
        // The Silverware rendering bridge maps this value to the engine's
        // RTextureHandle ID. Do not use output->nativeTexture here.
        const Silverware::FExternalTextureReference texture{
            static_cast<std::uint64_t>(output->textureHandle.id)
        };

        if (!m_ViewportImage->SetExternalTexture(
                texture, output->width, output->height))
        {
            std::cerr << "[SilverwareRenderer]: Failed to assign viewport texture.\n";
            m_ViewportImage->ClearSource();
        }
    }
    else
    {
        m_ViewportImage->ClearSource();
    }

    // Keep the viewport controller alive and rendering even when no mouse
    // input is being supplied by Silverware yet.
    SubmitViewportInput(viewportSubsystem);

    if (!m_Backend->Synchronize(
            *m_RootWidget,
            Silverware::EBackendSyncMode::Incremental))
        std::cerr << "[SilverwareRenderer]: Incremental synchronization failed.\n";
}

void SilverwareEditorRenderer::Shutdown()
{
    if (m_EditorHost)
        m_EditorHost->GetSubsystem<ViewportSubsystem>().Destroy(kViewportPanelKey);

    m_ViewportImage = nullptr;
    m_RootWidget.reset();

    m_Backend = nullptr;
    m_EditorRuntime = nullptr;
    m_EditorHost = nullptr;
}