//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>

#include "EditorEdits/UndoableActions/IUndoableAction.h"
#include "Core/Math/FMath.h"

class EditorRuntime;

class SetActorTransformAction : public IUndoableAction
{
public:
    using ActorID = uint64_t;

private:
    EditorRuntime& m_Runtime;
    ActorID m_Target = 0;

    FTransform m_Before;
    FTransform m_After;

public:
    SetActorTransformAction(EditorRuntime& runtime, ActorID target,
                            const FTransform& before, const FTransform& after)
        : m_Runtime(runtime)
        , m_Target(target)
        , m_Before(before)
        , m_After(after)
    {}

    void Do() override;
    void Undo() override;

    [[nodiscard]] std::string GetTitle() const override { return "Changed Transform"; }
    EEditEffect GetEffects() const override { return EEditEffect::Inspector; }
};