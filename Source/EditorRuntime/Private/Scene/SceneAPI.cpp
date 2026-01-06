//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Scene/SceneAPI.h"

#include "Core/EngineContext.h"
#include "Framework/SceneManager.h"
#include "Scene/JScene.h"

EditorSceneAPI::EditorSceneAPI(EngineContext &ctx, SceneManager &scene, DebugDraw &debugDraw):
m_Context(ctx),
m_SceneManager(scene),
m_DebugDraw(debugDraw)
{
}

JScene* EditorSceneAPI::GetActiveScene()
{
    return m_SceneManager.GetActiveScene();
}

DebugDraw& EditorSceneAPI::GetDebugDraw()
{
    return m_DebugDraw;
}

bool EditorSceneAPI::TryGetActorWorldTransform(ActorID id, FTransform& outXf) const
{
    JActor* a = m_SceneManager.FindActorByID(id);
    if (!a) return false;

    outXf = a->GetActorTransform();
    return true;
}

std::vector<FHierarchySnapshot> EditorSceneAPI::BuildHierarchySnapshot() const
{
    std::vector<FHierarchySnapshot> result;

    JScene* scene = m_SceneManager.GetActiveScene();
    if (!scene)
        return result;

    auto actors = m_SceneManager.ListAllActors();
    result.reserve(actors.size());

    for (auto* actor : actors)
    {
        if (!actor) continue;

        FHierarchySnapshot info{};
        info.id = actor->GetRuntimeID();
        info.parentID = actor->GetParentActor()
                            ? actor->GetParentActor()->GetRuntimeID()
                            : 0;
        info.name = actor->GetName();
        info.hasChildren = !actor->GetChildActors().empty();

        result.push_back(std::move(info));
    }

    return result;
}

void EditorSceneAPI::SetSelectedActors(const std::vector<ActorID> &ids)
{
    m_SelectedActors = ids;

    // Push to render-facing state (so renderer can outline)
    m_Context.GetEditorSelectionState().SetSelectedActors(ids);
}

void EditorSceneAPI::DeleteActors(const std::vector<ActorID> &ids)
{
    for (ActorID id : ids)
        m_SceneManager.ImmediateDestroyActor(id);
}

void EditorSceneAPI::DuplicateActors(const std::vector<ActorID> &ids)
{
    // TODO: implement cloning later
}

bool EditorSceneAPI::RaycastIntoTheScene(const FRay& ray, FRaycastHit& outHit) // TODO: Should detect based on AABB or mesh for future
{
    outHit = FRaycastHit{};

    JScene* scene = m_SceneManager.GetActiveScene();
    if (!scene)
        return false;

    const auto actors = m_SceneManager.ListAllActors();
    if (actors.empty())
        return false;

    const float pickRadius = 1.5f;  // we tweak this for how "fat" the ray feels
    const float pickRadiusSq = pickRadius * pickRadius;

    const float minDistance = 0.01f; // ignore stuff basically at the ray position

    bool bFound = false;
    float bestDistance = std::numeric_limits<float>::max();

    for (JActor* actor : actors)
    {
        if (!actor)
            continue;

        if (!actor->IsVisible()) continue;

        // Actor world position
        const FVector3 actorPos = actor->GetActorLocation();

        // Vector from ray origin to actor
        const FVector3 toActor = actorPos - ray.origin;

        // Project onto ray dir to get t of closest point on ray to the actor center
        const float t = FMath::Dot(toActor, ray.direction);
        if (t < minDistance)
            continue; // behind or too close

        const FVector3 closestPoint = ray.origin + ray.direction * t;

        // Distance from actor center to ray
        const float distSq = (actorPos - closestPoint).LengthSquared();
        if (distSq > pickRadiusSq)
            continue; // too far from ray

        // Choose the closest valid hit (smallest t)
        if (!bFound || t < bestDistance)
        {
            bFound = true;
            bestDistance = t;

            outHit.bHit = true;
            outHit.actorID = actor->GetRuntimeID();
            outHit.distance = t;
            outHit.position = closestPoint;
            // outHit.Normal   = ... // leave empty for now
        }
    }

    return bFound;
}