//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>

using ActorID = uint64_t;

class EngineContext;
class EditorCore;

class EditorContext
{
private:
    EngineContext& m_EngineState;

    std::vector<ActorID> m_SelectedActors;
    bool m_IsUIActive = false;

    EditorCore* m_Core = nullptr; // Non-owning pointer
public:
    explicit EditorContext(EngineContext& engineState) : m_EngineState(engineState) {}

    EngineContext& GetState() { return m_EngineState; }
    [[nodiscard]] const EngineContext& GetState() const { return m_EngineState; }

    // --- Selection ---

    void SetSelectedActor(ActorID actorId)
    {
        m_SelectedActors.clear();
        if (actorId >= 0)
            m_SelectedActors.push_back(actorId);
    }

    [[nodiscard]] const std::vector<ActorID>& GetSelectedActors() const { return m_SelectedActors; }

    [[nodiscard]] ActorID GetPrimarySelectedActor() const
    {
        return m_SelectedActors.empty() ? -1 : m_SelectedActors.front();
    }

    [[nodiscard]] bool IsActorSelected(ActorID id) const
    {
        for (auto selected : m_SelectedActors)
            if (selected == id) return true;
        return false;
    }

    // Core linkage

    void SetCore(EditorCore* core) { m_Core = core; }
    EditorCore* GetCore() { return m_Core; }
    const EditorCore* GetCore() const { return m_Core; }

    // UI state

    void SetIsUIActive(bool active) { m_IsUIActive = active; }
    bool GetIsUIActive() const { return m_IsUIActive; }
};
