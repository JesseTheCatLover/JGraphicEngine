// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareEditorRenderer.h"

#include "EditorCore/EditorHost.h"
#include "EditorRuntime.h"
#include "Backends/SilverwareBackend.h"

#include "Panels/Controllers/Inputs/FViewportPanelInput.h"
#include "Panels/Controllers/Outputs/FViewportOutput.h"
#include "Panels/Subsystems/ViewportSubsystem.h"

#include "Silverware/Backend/IWidgetBackend.h"
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

namespace
{
    constexpr char kViewportPanelKey[] = "Viewport:0";

    constexpr float kViewportImageHeight = 480.0f;

    void SubmitViewportInput(ViewportSubsystem& viewportSubsystem, float width, float height)
    {
        const bool bHasValidSize = width > 0.0f && height > 0.0f;

        FViewportPanelInput input{};
        input.panelKey = kViewportPanelKey;
        input.width = bHasValidSize ? width : 0.0f;
        input.height = bHasValidSize ? height : 0.0f;
        input.bAppFocused = true;
        input.bHidden = !bHasValidSize;

        // No input interaction submitted yet
        viewportSubsystem.SubmitInput(input);
    }

    bool AddTextSample(
        Silverware::ContainerWidget& parent,
        const char* text,
        float fontSize,
        const Silverware::FColor& color,
        Silverware::EFontWeight fontWeight,
        float topMargin)
    {
        using namespace Silverware;

        auto label = Silverware::MakeUnique<TextWidget>(text);

        auto layout = label->GetLayout();
        layout.margin.left = FLayoutLength::Pixels(8.0f);
        layout.margin.top = FLayoutLength::Pixels(topMargin);
        label->SetLayout(layout);

        FTextStyle style;
        style.fontFamily = "LatoLatin";
        style.fontSize = fontSize;
        style.color = color;
        style.fontWeight = fontWeight;
        label->SetTextStyle(style);

        return parent.AddChild(Silverware::TUniquePtr<CoreWidget>(label.release()));
    }

    bool AddHierarchyRow(
        Silverware::ContainerWidget& parent,
        const char* text,
        bool selected,
        bool disabled)
    {
        using namespace Silverware;

        auto row = Silverware::MakeUnique<ContainerWidget>();

        auto layout = row->GetLayout();
        layout.width = FLayoutLength::Percent(100.0f);
        layout.height = FLayoutLength::Pixels(34.0f);
        row->SetLayout(layout);

        FContainerLayout containerLayout;
        containerLayout.direction = ELayoutDirection::Horizontal;
        containerLayout.gap = FLayoutLength::Pixels(4.0f);
        containerLayout.alignItems = EAlignment::Start;
        containerLayout.justifyContent = EJustification::Start;
        row->SetContainerLayout(containerLayout);

        FContainerStyle style;
        style.backgroundColor = selected
            ? FColor{0.18f, 0.29f, 0.44f, 1.0f}
            : FColor{0.15f, 0.15f, 0.17f, 1.0f};
        style.cornerRadius = 5.0f;
        style.opacity = disabled ? 0.45f : 1.0f;

        if (selected)
        {
            style.borderColor = FColor{0.35f, 0.58f, 0.92f, 1.0f};
            style.borderWidth = 1.0f;
            style.borderStyle = EBorderStyle::Solid;
        }
        else
        {
            style.borderStyle = EBorderStyle::None;
        }

        row->SetContainerStyle(style);

        const FColor textColor = disabled
            ? FColor{0.50f, 0.50f, 0.53f, 1.0f}
            : FColor{0.90f, 0.90f, 0.92f, 1.0f};

        if (!AddTextSample(
                *row,
                text,
                15.0f,
                textColor,
                selected ? EFontWeight::Bold : EFontWeight::Normal,
                5.0f))
            return false;

        return parent.AddChild(Silverware::TUniquePtr<CoreWidget>(row.release()));
    }

    bool AddEditorSection(
        Silverware::ContainerWidget& root,
        const char* title,
        float widthPercent,
        Silverware::ImageWidget** outImage = nullptr,
        Silverware::ContainerWidget** outSection = nullptr)
    {
        using namespace Silverware;

        if (outImage)
            *outImage = nullptr;

        if (outSection)
            *outSection = nullptr;

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
            imageLayout.flexGrow = 1.f;
            image->SetLayout(imageLayout);

            FImageStyle imageStyle;
            imageStyle.fit = EImageFit::Cover;
            imageStyle.opacity = 1.0f;
            imageStyle.cornerRadius = 12.0f;
            image->SetImageStyle(imageStyle);

            imageWidget = image.get();

            if (!section->AddChild(Silverware::TUniquePtr<CoreWidget>(image.release())))
                return false;
        }

        ContainerWidget* sectionWidget = section.get();

        if (!root.AddChild(Silverware::TUniquePtr<CoreWidget>(section.release())))
            return false;

        if (outImage)
            *outImage = imageWidget;

        if (outSection)
            *outSection = sectionWidget;

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

    /// Register the viewport controller before the first layout measurement.
    // The render target will be created once the widget has valid dimensions.
    SubmitViewportInput(editorHost.GetSubsystem<ViewportSubsystem>(), 0.0f, 0.0f);

    if (!m_Backend->Synchronize(*m_RootWidget, Silverware::EBackendSyncMode::Full))
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

    using namespace Silverware;

    ContainerWidget* hierarchySection = nullptr;
    ContainerWidget* inspectorSection = nullptr;

    if (!AddEditorSection(
            *m_RootWidget,
            "Scene Hierarchy",
            20.0f,
            nullptr,
            &hierarchySection) ||
        !AddEditorSection(
            *m_RootWidget,
            "Viewport",
            55.0f,
            &m_ViewportImage) ||
        !AddEditorSection(
            *m_RootWidget,
            "Inspector",
            20.0f,
            nullptr,
            &inspectorSection))
        return false;

    // Scene Hierarchy: default, selected, and dimmed rows.
    if (!AddHierarchyRow(*hierarchySection, "Camera", false, false) ||
        !AddHierarchyRow(*hierarchySection, "Directional Light", true, false) ||
        !AddHierarchyRow(*hierarchySection, "Cube", false, true))
        return false;

    // Inspector: experiment with text size, weight, and color.
    const FColor primaryText{0.92f, 0.92f, 0.94f, 1.0f};
    const FColor secondaryText{0.65f, 0.66f, 0.70f, 1.0f};
    const FColor accentText{0.45f, 0.68f, 1.0f, 1.0f};

    if (!AddTextSample(
            *inspectorSection,
            "Transform",
            20.0f,
            primaryText,
            EFontWeight::Bold,
            4.0f) ||
        !AddTextSample(
            *inspectorSection,
            "Position   0, 0, 0",
            14.0f,
            secondaryText,
            EFontWeight::Normal,
            2.0f) ||
        !AddTextSample(
            *inspectorSection,
            "Rotation   0, 0, 0",
            14.0f,
            secondaryText,
            EFontWeight::Normal,
            0.0f) ||
        !AddTextSample(
            *inspectorSection,
            "Scale      1, 1, 1",
            14.0f,
            secondaryText,
            EFontWeight::Normal,
            0.0f) ||
        !AddTextSample(
            *inspectorSection,
            "Mesh Renderer",
            18.0f,
            accentText,
            EFontWeight::Bold,
            12.0f) ||
        !AddTextSample(
            *inspectorSection,
            "No renderer attached",
            14.0f,
            secondaryText,
            EFontWeight::Normal,
            0.0f))
        return false;

    return true;
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
                texture,
                output->width,
                output->height))
        {
            std::cerr << "[SilverwareRenderer]: Failed to assign viewport texture.\n";
            m_ViewportImage->ClearSource();
        }
    }
    else
    {
        m_ViewportImage->ClearSource();
    }

    float viewportWidth = 0.0f;
    float viewportHeight = 0.0f;

    if (!m_Backend->TryGetWidgetSize(*m_ViewportImage, viewportWidth, viewportHeight))
    {
        viewportWidth = 0.0f;
        viewportHeight = 0.0f;
    }

    SubmitViewportInput(viewportSubsystem, viewportWidth, viewportHeight);

    if (!m_Backend->Synchronize( *m_RootWidget, Silverware::EBackendSyncMode::Incremental))
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