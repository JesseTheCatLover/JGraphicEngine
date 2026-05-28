//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "SceneSelectionSynchronizer.h"

#include "Core/EditorHost.h"
#include "SelectionService.h"
#include "EditorRuntime.h"

SceneSelectionSynchronizer::SceneSelectionSynchronizer(EditorHost& host, EditorRuntime& runtime)
    : m_Host(host)
    , m_Runtime(runtime)
{
    Bind();
}

void SceneSelectionSynchronizer::Bind()
{
    auto& model = m_Host.GetService<SelectionService>().GetSceneActorSelection();

    auto* delegate = &model.OnChanged();
    auto handle = delegate->AddLambda([this]()
    {
        SyncNow();
    });

    m_SelectionChangedSub = TDelegateSubscription<TMulticastDelegate<>>(delegate, handle);

    // Optional: sync immediately on bind so runtime reflects current editor selection
    SyncNow();
}

void SceneSelectionSynchronizer::Unbind()
{
    m_SelectionChangedSub.Reset();
}

void SceneSelectionSynchronizer::SyncNow()
{
    const auto& selected = m_Host.GetService<SelectionService>().GetSceneActorSelection().GetSelection();
    m_Runtime.GetScene().SetSelectedActors(selected);
}