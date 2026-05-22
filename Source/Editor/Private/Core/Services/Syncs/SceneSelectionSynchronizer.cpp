//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "SceneSelectionSynchronizer.h"

#include "Core/EditorHost.h"
#include "Core/Services/SelectionService.h"
#include "EditorRuntime.h"

SceneSelectionSynchronizer::SceneSelectionSynchronizer(EditorHost& host, EditorRuntime& runtime)
    : m_Host(host)
    , m_Runtime(runtime)
{
    Bind(m_Host);
}

SceneSelectionSynchronizer::~SceneSelectionSynchronizer()
{
    Unbind(m_Host);
}

void SceneSelectionSynchronizer::Bind(EditorHost& host)
{
    auto& model = host.GetService<SelectionService>().GetSceneActorSelection();

    if (m_ListenerID != 0)
        return;

    m_ListenerID = model.AddChangedListener([this, &host]()
    {
        SyncNow(host);
    });
}

void SceneSelectionSynchronizer::Unbind(EditorHost& host)
{
    if (m_ListenerID == 0)
        return;

    auto& model = host.GetService<SelectionService>().GetSceneActorSelection();
    model.RemoveChangedListener(m_ListenerID);
    m_ListenerID = 0;
}

void SceneSelectionSynchronizer::SyncNow(EditorHost& host)
{
    const auto& selected = host.GetService<SelectionService>().GetSceneActorSelection().GetSelection();
    m_Runtime.GetScene().SetSelectedActors(selected);
}