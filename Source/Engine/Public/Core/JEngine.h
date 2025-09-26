//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "EngineState.h"
#include "ServiceContainer.h"

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
    ServiceContainer& GetServices() { return m_Services; }

    void Initialize();
    void Run();
    void Shutdown();

private:
    JEngine() : m_State(), m_Services() {}
    ~JEngine() = default;

    EngineState m_State;
    ServiceContainer m_Services;

};
