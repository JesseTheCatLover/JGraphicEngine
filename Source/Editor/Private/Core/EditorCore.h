//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <stack>

#include "EditorContext.h"
#include "EngineEditor.h"
#include "IEditorCommand.h"
#include "Core/Memory/SmartPointers.h"

struct FRenderView;
class IEditorPanel;
class EditorContext;
class FSelectionModifiers;

class EditorCore
{
    friend class EditorApp;
private:
    explicit EditorCore(EditorContext& context, EngineEditor& engineEditor);

    EditorContext& m_Context;
    EngineEditor& m_EngineEditor;

    FEditorFrameSnapshot m_FrameSnapshot;

    std::vector<FEditorActorSnapshot>  m_HierarchySnapshot;
    std::vector<ActorID> m_SelectedActors;
    ActorID m_SelectionAnchor = 0; // last “primary” clicked actor
    ActorID m_RevealInHierarchy = 0; //

    std::stack<TUniquePtr<IEditorCommand>> m_UndoStack;
    std::stack<TUniquePtr<IEditorCommand>> m_RedoStack;

    // One editor camera per viewport panel for now
    std::unordered_map<const IEditorPanel*, UDynamicID::IDType> m_PanelToCameraMap;

    // Which panel currently owns editor camera input
    const IEditorPanel* m_ActiveViewportPanel = nullptr;

    struct FViewportPanelState
    {
        float width  = 0.f;
        float height = 0.f;
    };

    // Per-panel viewport size (used to build FCameraToolState)
    std::unordered_map<const IEditorPanel*, FViewportPanelState> m_CameraStateMap;

    // Per-camera MSAA samples (1 = no MSAA)
    std::unordered_map<UDynamicID::IDType, int> m_CameraSampleMap;

    // // Internal helper to sync selection to EngineEditor
    // void SyncSelectionToEngine();
    // void UpdateSelectionFlagsInHierarchy();

    // Helpers:
    void PushFrameInfoToEditorContext();
    void TickEditorTools(float deltaTime);
    void TickCameraTools(FCameraToolState& cameraState);
    void ClearFrameStates();

    // Tools
    void ActivateCameraForPanel(const IEditorPanel* panel);
    void DeactivateCameraForPanel(const IEditorPanel* panel);

public:
    EditorContext& GetContext() { return m_Context; }
    [[nodiscard]] const EditorContext& GetContext() const { return m_Context; }

    // Called every frame from EditorApp::OnTick
    void Tick(float deltaTime);

    // Scene Hierarchy
    void UpdateHierarchySnapshot();
    const std::vector<FEditorActorSnapshot>& GetHierarchySnapshot() const { return m_HierarchySnapshot; }

    // Viewport section
    [[nodiscard]] void* GetViewportTextureHandle(const IEditorPanel* panel) const;
    void PickActorAtViewportPos(const IEditorPanel* panel, float x, float y, const FSelectionModifiers& mods);

    // Called by panels that want an editor camera
    void CreateCameraForPanel(const IEditorPanel* panel);
    void DestroyCameraForPanel(const IEditorPanel* panel);
    void SetViewportFocused(const IEditorPanel* panel, bool bFocused);
    // Called by panel to update its camera's aspect ratio
    void OnViewportResized(const IEditorPanel* panel, float width, float height);
    void SetViewportMSAASamples(const IEditorPanel* panel, int samples);

    // Command pipeline
    void ExecuteCommand(TUniquePtr<IEditorCommand> cmd);
    void Undo();
    void Redo();

    bool CanUndo() const { return !m_UndoStack.empty(); }
    bool CanRedo() const { return !m_RedoStack.empty(); }

    // Actor Selection

    // TODO: Should be promoted by commands later
    // Single click without modifiers
    void SelectSingleActor(ActorID id);

    // Ctrl/Cmd click -> toggle
    void ToggleActorSelection(ActorID id);

    // Shift+click behavior
    void SelectRangeTo(ActorID id);

    // Called by UI panels
    void HandleSelectionClick(ActorID id, const FSelectionModifiers& mods);

    void ClearSelection();

    ActorID ConsumeRevealRequest()
    {
        ActorID id = m_RevealInHierarchy;
        m_RevealInHierarchy = 0;
        return id;
    }
};