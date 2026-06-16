//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/Memory/SmartPointers.h"

class IUndoableAction;

class IEditActionSink
{
public:
    virtual ~IEditActionSink() = default;

    // “submit” implies: execute now + record if needed
    virtual void Submit(TUniquePtr<IUndoableAction> action) = 0;
};