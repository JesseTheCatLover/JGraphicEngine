//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorEdits/UndoableActions/SetActorsTransformAction.h"
#include "EditorRuntime.h"

void SetActorsTransformAction::Do()
{
    auto& scene = m_Runtime.GetScene();
    const size_t n = std::min(m_Actors.size(), m_After.size());

    for (size_t i = 0; i < n; ++i)
        scene.TrySetActorWorldTransform(m_Actors[i], m_After[i]);
}

void SetActorsTransformAction::Undo()
{
    auto& scene = m_Runtime.GetScene();
    const size_t n = std::min(m_Actors.size(), m_Before.size());

    for (size_t i = 0; i < n; ++i)
        scene.TrySetActorWorldTransform(m_Actors[i], m_Before[i]);
}