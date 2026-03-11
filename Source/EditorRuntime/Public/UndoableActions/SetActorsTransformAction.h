//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "UndoableActions/IUndoableAction.h"
#include "Core/Math/FTransform.h"

class EditorRuntime;

class SetActorsTransformAction : public IUndoableAction
{
public:
    using ActorID = uint64_t;

private:
    EditorRuntime& m_Runtime;
    std::vector<ActorID>   m_Actors;
    std::vector<FTransform> m_Before;
    std::vector<FTransform> m_After;

    std::string m_Title;

public:
    SetActorsTransformAction(EditorRuntime& rt,
                             std::vector<ActorID> actors,
                             std::vector<FTransform> before,
                             std::vector<FTransform> after,
                             std::string title = "Transform Actors")
        : m_Runtime(rt)
        , m_Actors(std::move(actors))
        , m_Before(std::move(before))
        , m_After(std::move(after))
        , m_Title(std::move(title))
    {}

    void Do() override;
    void Undo() override;

    [[nodiscard]] std::string GetTitle() const override { return m_Title; }
    EEditEffect GetEffects() const override { return EEditEffect::Viewport | EEditEffect::Inspector; }
};