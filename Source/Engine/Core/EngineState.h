//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "Engine/Scene/JActor.h"

class EngineState
{
public:
    // Scene management TODO: Create a scene structure for future to implement this
    //void SetActiveScene(std::shared_ptr<JScene> scene);
    //std::shared_ptr<JScene> GetActiveScene() const;

    // For now we keep the SceneActors directly here
    std::vector<JActor>& GetSceneActors() { return  m_SceneActors; }
    const std::vector<JActor>& GetSceneActors() const { return m_SceneActors;}

    // Frame timing
    void SetDeltaTime(float dt) { m_DeltaTime = dt; }
    float GetDeltaTime() const { return m_DeltaTime; }

    // Engine running flags
    void SetRunning(bool running) { m_Running = running; }
    bool IsRunning() const { return m_Running; }

private:
    EngineState() = default;
    ~EngineState() = default;

    // Disallow copy/move
    EngineState(const EngineState&) = delete;
    EngineState& operator=(const EngineState&) = delete;

    friend class JEngine; // only JEngine can create EngineState

    // std::shared_ptr<JScene> m_ActiveScene;
    std::vector<JActor> m_SceneActors;
    float m_DeltaTime = 0.f;
    bool m_Running = true;
};
