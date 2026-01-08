//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>

#include "FHierarchySnapshot.h"
#include "FRaycast.h"

struct FQuat;
struct FTransform;
class DebugDraw;
class JScene;
class SceneManager;
class EngineContext;

class EditorSceneAPI
{
private:
    using ActorID = uint64_t;

    EngineContext& m_Context;
    SceneManager& m_SceneManager;
    DebugDraw& m_DebugDraw;

    std::vector<ActorID> m_SelectedActors;

public:
    EditorSceneAPI(EngineContext& ctx, SceneManager& scene, DebugDraw& debugDraw);

    JScene* GetActiveScene();
    DebugDraw& GetDebugDraw();

    bool TryGetActorWorldTransform(ActorID id, FTransform& outXf) const;
    bool TrySetActorWorldTransform(ActorID id, const FTransform& xf);
    bool TrySetActorWorldLocation(ActorID id, const FVector3& p);
    bool TrySetActorWorldRotation(ActorID id, const FQuat& q);
    bool TrySetActorWorldScale(ActorID id, const FVector3& s);

    [[nodiscard]] std::vector<FHierarchySnapshot>
        BuildHierarchySnapshot() const;
    void SetSelectedActors(const std::vector<ActorID>& ids);
    void DeleteActors(const std::vector<ActorID>& ids);
    void DuplicateActors(const std::vector<ActorID>& ids);

    bool RaycastIntoTheScene(const FRay& ray, FRaycastHit& outHit);
};
