//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

enum class EEditEffect : uint32_t
{
    None        = 0,
    Hierarchy   = 1u << 0,
    Inspector   = 1u << 1,
    Selection   = 1u << 2,
    Viewport    = 1u << 3,
};

inline EEditEffect operator|(EEditEffect a, EEditEffect b)
{
    return (EEditEffect)((uint32_t)a | (uint32_t)b);
}
inline bool HasEffect(EEditEffect v, EEditEffect f)
{
    return (((uint32_t)v) & ((uint32_t)f)) != 0;
}

class IUndoableAction
{
public:
    virtual ~IUndoableAction() = default;

    virtual void Do() = 0;
    virtual void Undo() = 0;

    // For history/timeline UI
    [[nodiscard]] virtual std::string GetTitle() const = 0;

    [[nodiscard]] virtual EEditEffect GetEffects() const { return EEditEffect::None; }
};