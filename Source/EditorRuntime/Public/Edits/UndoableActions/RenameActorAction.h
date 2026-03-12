//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <cstdint>

#include "Edits/UndoableActions/IUndoableAction.h"

class EditorRuntime;

class RenameActorAction : public IUndoableAction
{
public:
    using ActorID = uint64_t;

private:
    EditorRuntime& m_Runtime;
    ActorID m_Target = 0;

    std::string m_OldName;
    std::string m_NewName;

public:
    RenameActorAction(EditorRuntime& runtime, ActorID target, std::string oldName, std::string newName)
        : m_Runtime(runtime)
        , m_Target(target)
        , m_OldName(std::move(oldName))
        , m_NewName(std::move(newName))
    {}
    ~RenameActorAction();

    void Do() override;
    void Undo() override;

    [[nodiscard]] std::string GetTitle() const override { return "Rename Actor"; }

    EEditEffect GetEffects() const override { return EEditEffect::Hierarchy | EEditEffect::Inspector; }
};