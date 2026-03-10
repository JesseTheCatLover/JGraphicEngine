//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

class IUndoableAction
{
public:
    virtual ~IUndoableAction() = default;

    virtual void Do() = 0;
    virtual void Undo() = 0;

    // For history/timeline UI
    [[nodiscard]] virtual std::string GetTitle() const = 0;
};