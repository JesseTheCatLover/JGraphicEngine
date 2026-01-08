//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>

#include "Core/IEditorService.h"
#include "Scene/FHierarchySnapshot.h"

struct FQuat;
struct FVector3;
struct FTransform;
class EditorRuntime;
struct FRaycastHit;
struct FRay;

class SceneQueryService : public IEditorService
{
    using ActorID = uint64_t;
private:
    EditorRuntime& m_Runtime;

public:
    explicit SceneQueryService(EditorRuntime& runtime);

    bool RaycastIntoTheScene(const FRay& ray, FRaycastHit& outHit);

    bool TryGetActorWorldTransform(ActorID id, FTransform& outXf) const;

    std::vector<FHierarchySnapshot> BuildHierarchySnapshot();

    // Returns false if actor doesn't exist.
    bool TrySetActorWorldTransform(ActorID id, const FTransform& xf);
    bool TrySetActorWorldLocation(ActorID id, const FVector3& p);
    bool TrySetActorWorldRotation(ActorID id, const FQuat& q);
    bool TrySetActorWorldScale(ActorID id, const FVector3& s);
};
