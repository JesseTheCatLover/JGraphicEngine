// //  Copyright 2025 JesseTheCatLover. All Rights Reserved.
//
// #pragma once
// #include <vector>
// #include <unordered_set>
// #include <cstdint>
//
// #include "UI/IEditorPanels.h"
//
// using ActorID = uint64_t;
//
// struct FEditorActorSnapshot;
// class EditorContext;
//
// class SceneHierarchyPanel : public IEditorPanel
// {
// private:
//     bool bClickedAnyItemThisFrame = false;
//
//     // Persistent UI state:
//     std::unordered_set<ActorID> m_OpenNodes;
//     ActorID m_ScrollTo = 0;
//
//     void DrawActorNode(
//     const FEditorActorSnapshot& node,
//     const std::vector<FEditorActorSnapshot>& allActors,
//     EditorHost& core);
//
//     void ApplyRevealRequest(
//         const std::vector<FEditorActorSnapshot>& actors,
//         EditorHost& core);
//
// public:
//     const char* GetName() const override { return "Scene Hierarchy"; }
//
//     void Draw(EditorContext& context, EditorHost& core);
// };