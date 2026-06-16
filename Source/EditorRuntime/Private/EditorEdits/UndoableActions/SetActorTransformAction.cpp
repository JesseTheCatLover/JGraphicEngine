//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorEdits/UndoableActions/SetActorTransformAction.h"
#include "EditorRuntime.h"

void SetActorTransformAction::Do()
{
    m_Runtime.GetScene().TrySetActorWorldTransform(m_Target, m_After);
}

void SetActorTransformAction::Undo()
{
    m_Runtime.GetScene().TrySetActorWorldTransform(m_Target, m_Before);
}