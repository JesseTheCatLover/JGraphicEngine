//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <stack>

#include "EditorContext.h"
#include "EngineEditor.h"
#include "IEditorCommand.h"
#include "Core/Memory/SmartPointers.h"

class IEditorPanel;
class EditorContext;

class EditorCore
{
private:
    EditorContext& m_Context;
    EngineEditor& m_EngineEditor;

    FEditorFrameSnapshot m_FrameSnapshot;
    std::vector<FEditorActorSnapshot>  m_HierarchySnapshot;

    std::stack<TUniquePtr<IEditorCommand>> m_UndoStack;
    std::stack<TUniquePtr<IEditorCommand>> m_RedoStack;

    // One editor camera per viewport panel for now
    std::unordered_map<const IEditorPanel*, UDynamicID::IDType> m_PanelToCameraMap;

    // Which panel currently owns editor camera input
    const IEditorPanel* m_ActiveViewportPanel = nullptr;
    std::unordered_map<UDynamicID::IDType, float> m_CameraAspectMap;

    // // Internal helper to sync selection to EngineEditor
    // void SyncSelectionToEngine();
    // void UpdateSelectionFlagsInHierarchy();

    // Helpers:
    void PushFrameInfoToEditorContext();
    void TickEditorTools(float deltaTime);
    void SubmitEditorViewSources();
    void ClearFrameStates();

    // Tools
    void ActivateCameraForPanel(const IEditorPanel* panel);
    void DeactivateCameraForPanel(const IEditorPanel* panel);

public:
    explicit EditorCore(EditorContext& context, EngineEditor& engineEditor);

    EditorContext& GetContext() { return m_Context; }
    [[nodiscard]] const EditorContext& GetContext() const { return m_Context; }

    // Called every frame from EditorApp::OnTick
    void Tick(float deltaTime);

    // -------- Selection API (used by panels) --------
    void SelectSingle(ActorID id);          // replace selection with this actor
    void AddToSelection(ActorID id);        // add if not already selected
    void ToggleSelection(ActorID id);       // toggle membership
    void ClearSelection();

    const std::vector<ActorID>& GetSelection() const;

    // -------- Scene operations --------
    void DeleteSelectedActors();
    // later: DuplicateSelectedActors(), FocusOnSelection(), etc.

    // -------- Gizmo/tools --------
    void SetGizmoMode(EGizmoMode mode);
    EGizmoMode GetGizmoMode() const;

    const std::vector<FEditorActorSnapshot>& GetHierarchySnapshot() const { return m_HierarchySnapshot; }

    // Viewport section
    [[nodiscard]] void* GetViewportTextureHandle() const;

    // Called by panels that want an editor camera
    void CreateCameraForPanel(const IEditorPanel* panel);
    void DestroyCameraForPanel(const IEditorPanel* panel);
    void SetViewportFocused(const IEditorPanel* panel, bool bFocused);
    // Called by panel to update its camera's aspect ratio
    void OnViewportResized(const IEditorPanel* panel, float aspectRatio);
    void SetSizeTemp(float width, float height);

    // Command pipeline
    void ExecuteCommand(TUniquePtr<IEditorCommand> cmd);
    void Undo();
    void Redo();

    bool CanUndo() const { return !m_UndoStack.empty(); }
    bool CanRedo() const { return !m_RedoStack.empty(); }

    // Simple selection helper (can be promoted to command later).
    void SelectActor(int actorId);
};