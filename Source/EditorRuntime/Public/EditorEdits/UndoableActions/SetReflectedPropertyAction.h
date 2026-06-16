//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>

#include "EditorEdits/UndoableActions/IUndoableAction.h"
#include "Core/Reflection/RETypeRegistry.h"

class EditorRuntime;

class SetReflectedPropertyAction : public IUndoableAction
{
private:
    EditorRuntime& m_Runtime;

    uint64_t    m_ContextActorID = 0;
    std::string m_ObjectUUID;
    std::string m_DeclaringTypeName;
    std::string m_PropName;

    REVariant   m_Before;
    REVariant   m_After;

    std::string m_Title;

public:
    SetReflectedPropertyAction(EditorRuntime& rt,
                               uint64_t contextActorID,
                               std::string objectUUID,
                               std::string declaringTypeName,
                               std::string propName,
                               REVariant before,
                               REVariant after,
                               std::string title = {});

    ~SetReflectedPropertyAction() override = default;

    void Do() override;
    void Undo() override;

    [[nodiscard]] std::string GetTitle() const override { return m_Title; }

    EEditEffect GetEffects() const override { return EEditEffect::Inspector; }
};