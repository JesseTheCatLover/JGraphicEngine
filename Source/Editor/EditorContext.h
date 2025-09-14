class EngineState;

class EditorContext {
public:
    EditorContext(EngineState& engineState) : m_EngineState(engineState) {}

    EngineState& GetState() { return m_EngineState; }
    const EngineState& GetState() const { return m_EngineState; }

    void SetSelectedActor(int actorId) {
        m_SelectedActor = actorId;
    }

    int GetSelectedActor() const { return m_SelectedActor; }

    bool GetIsUIActive() const { return m_IsUIActive; }

private:
    EngineState& m_EngineState;
    int m_SelectedActor = -1;
    bool m_IsUIActive = false;
};
