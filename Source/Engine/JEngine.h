//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/EngineState.h"

class EditorContext;

class JEngine
{
public:
    static JEngine& Get()
    {
        static JEngine instance;
        return instance;
    }

    // Non-copyable, non-movable
    JEngine(const JEngine&) = delete;
    JEngine& operator=(const JEngine&) = delete;

    EngineState& GetState() { return m_State; }

private:
    JEngine() = default;
    ~JEngine() = default;

    EngineState m_State;
};
