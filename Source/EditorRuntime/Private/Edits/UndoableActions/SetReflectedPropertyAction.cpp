//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Edits/UndoableActions/SetReflectedPropertyAction.h"

#include "EditorRuntime.h"
#include "Scene/SceneAPI.h"

SetReflectedPropertyAction::SetReflectedPropertyAction(EditorRuntime& rt,
                                                       uint64_t contextActorID,
                                                       std::string objectUUID,
                                                       std::string declaringTypeName,
                                                       std::string propName,
                                                       REVariant before,
                                                       REVariant after,
                                                       std::string title)
    : m_Runtime(rt)
    , m_ContextActorID(contextActorID)
    , m_ObjectUUID(std::move(objectUUID))
    , m_DeclaringTypeName(std::move(declaringTypeName))
    , m_PropName(std::move(propName))
    , m_Before(std::move(before))
    , m_After(std::move(after))
{
    if (!title.empty())
        m_Title = std::move(title);
    else
        m_Title = "Edit " + m_PropName;
}

void SetReflectedPropertyAction::Do()
{
    m_Runtime.GetScene().TryWriteReflectedProperty(
        m_ContextActorID,
        m_ObjectUUID,
        m_DeclaringTypeName,
        m_PropName,
        m_After
    );
}

void SetReflectedPropertyAction::Undo()
{
    m_Runtime.GetScene().TryWriteReflectedProperty(
        m_ContextActorID,
        m_ObjectUUID,
        m_DeclaringTypeName,
        m_PropName,
        m_Before
    );
}