//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <stack>

#include "EditorContext.h"
#include "EditorRuntime.h"
#include "EditorPanelManager.h"
#include "IEditorCommand.h"
#include "Core/Memory/SmartPointers.h"

struct FRenderView;
class IEditorPanel;
class EditorContext;
class FSelectionModifiers;

class EditorHost
{
    friend class EditorApp;
private:
    explicit EditorHost(EditorContext& context, EditorRuntime& runtime);

    EditorContext& m_Context;
    EditorRuntime& m_EngineEditor;

    EditorPanelManager m_PanelManager;

    std::vector<FEditorActorSnapshot>  m_HierarchySnapshot;
    std::vector<ActorID> m_SelectedActors;
    ActorID m_SelectionAnchor = 0; // last “primary” clicked actor
    ActorID m_RevealInHierarchy = 0; //

    std::stack<TUniquePtr<IEditorCommand>> m_UndoStack;
    std::stack<TUniquePtr<IEditorCommand>> m_RedoStack;

    // Which panel currently owns editor camera input
    const IEditorPanel* m_ActiveViewportPanel = nullptr;

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
    [[nodiscard]] const std::vector<FEditorActorSnapshot>& GetHierarchySnapshot() const { return m_HierarchySnapshot; }

    // --- Viewport section ---

    void SubmitViewportPanelContext(const FViewportPanelContext& ctx)
    {
        m_PanelManager.SubmitViewportPanelContext(ctx);
    }

    [[nodiscard]] void* GetViewportNativeTexture(const char* panelKey) const
    {
        return m_PanelManager.GetViewportNativeTexture(panelKey);
    }

    void DestroyViewport(const char* panelKey)
    {
        m_PanelManager.DestroyViewport(panelKey);
    }

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