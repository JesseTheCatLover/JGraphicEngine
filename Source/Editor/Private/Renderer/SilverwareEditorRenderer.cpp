// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareEditorRenderer.h"

#include "EditorCore/EditorHost.h"
#include "EditorRuntime.h"
#include "Backends/SilverwareBackend.h"

#include "Silverware/Backend/IWidgetBackend.h"
#include "Silverware/Core/ContainerWidget.h"
#include "Silverware/Core/CoreWidget.h"
#include "Silverware/Core/WidgetLayout.h"
#include "Silverware/Core/Styling/TextStyle.h"
#include "Silverware/UI/TextWidget.h"

#include <iostream>

namespace
{
    bool AddEditorSection(Silverware::ContainerWidget& root, const char* title, float widthPercent)
    {
        using namespace Silverware;

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

        auto label = Silverware::MakeUnique<TextWidget>(title);

        auto labelLayout = label->GetLayout();
        labelLayout.margin.top = FLayoutLength::Pixels(8.0f);
        labelLayout.margin.left = FLayoutLength::Pixels(8.0f);
        label->SetLayout(labelLayout);

        FTextStyle labelStyle;
        labelStyle.fontFamily = "Funnel Sans";
        labelStyle.fontSize = 30.0f;
        labelStyle.color = FColor{1.f, 1.f, 1.f, 1.0f};
        labelStyle.fontWeight = EFontWeight::Bold;
        label->SetTextStyle(labelStyle);

        if (!section->AddChild(Silverware::TUniquePtr<CoreWidget>(label.release())))
            return false;

        return root.AddChild(Silverware::TUniquePtr<CoreWidget>(section.release()));
    }
}

SilverwareEditorRenderer::SilverwareEditorRenderer()
{
}

SilverwareEditorRenderer::~SilverwareEditorRenderer()
{
    Shutdown();
}

bool SilverwareEditorRenderer::Initialize(EditorHost& editorHost, EditorRuntime& editorRuntime, SilverwareBackend& backend)
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

    m_RootWidget = MakeUnique<Silverware::ContainerWidget>();

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

    return
        AddEditorSection(*m_RootWidget, "Scene Hierarchy", 20.0f) &&
        AddEditorSection(*m_RootWidget, "Viewport", 55.0f) &&
        AddEditorSection(*m_RootWidget, "Inspector", 20.0f);
}

void SilverwareEditorRenderer::Tick(float deltaTime)
{
    (void)deltaTime;

    if (!m_Backend || !m_RootWidget)
        return;

    if (!m_Backend->Synchronize(*m_RootWidget, Silverware::EBackendSyncMode::Incremental))
        std::cerr << "[SilverwareRenderer]: Incremental synchronization failed.\n";
}

void SilverwareEditorRenderer::Shutdown()
{
    m_RootWidget.reset();

    m_Backend = nullptr;
    m_EditorRuntime = nullptr;
    m_EditorHost = nullptr;
}