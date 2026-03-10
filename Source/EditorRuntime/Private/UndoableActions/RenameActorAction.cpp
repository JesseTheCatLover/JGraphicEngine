//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "UndoableActions/RenameActorAction.h"
#include "EditorRuntime.h"

RenameActorAction::~RenameActorAction()
{
}

void RenameActorAction::Do()
{
    m_Runtime.GetScene().SetActorName(m_Target, m_NewName);
}

void RenameActorAction::Undo()
{
    m_Runtime.GetScene().SetActorName(m_Target, m_OldName);
}