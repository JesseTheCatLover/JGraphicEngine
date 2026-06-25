//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>
#include <string>

#include "FHierarchySnapshot.h"
#include "FRaycast.h"

struct REVariant;
class JCoreObject;
class JActor;
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

    JActor* TryGetActor(ActorID id) const;
    bool TryGetActorWorldTransform(ActorID id, FTransform& outXf) const;
    bool TrySetActorWorldTransform(ActorID id, const FTransform& xf);
    bool TrySetActorWorldLocation(ActorID id, const FVector3& p);
    bool TrySetActorWorldRotation(ActorID id, const FQuat& q);
    bool TrySetActorWorldScale(ActorID id, const FVector3& s);
    bool TryGetActorComponents(ActorID id, std::vector<JCoreObject*>& outObjects) const;

    [[nodiscard]] std::string GetActorName(ActorID id) const;
    void SetActorName(ActorID id, const std::string& newName);

    bool ReparentActor(ActorID childID, ActorID newParentID);
    bool ToggleActorVisibility(ActorID actorID);

    void DeleteActorsImmediately(const std::vector<ActorID> &ids);

    [[nodiscard]] std::vector<FHierarchySnapshot> BuildHierarchySnapshot() const;
    void SetSelectedActors(const std::vector<ActorID>& ids);

    void DeleteActors(const std::vector<ActorID>& ids);

    void DuplicateActors(const std::vector<ActorID>& ids);

    bool RaycastIntoTheScene(const FRay& ray, FRaycastHit& outHit);

    // --- Reflection mutation helpers (runtime-safe) ---
    JCoreObject* TryResolveObjectByUUID(ActorID contextActorID, const std::string& objectUUID) const;

    bool TryReadReflectedProperty(ActorID contextActorID,
                                  const std::string& objectUUID,
                                  const std::string& declaringTypeName,
                                  const std::string& propName,
                                  REVariant& outValue) const;

    bool TryWriteReflectedProperty(ActorID contextActorID,
                                   const std::string& objectUUID,
                                   const std::string& declaringTypeName,
                                   const std::string& propName,
                                   const REVariant& value);
};
