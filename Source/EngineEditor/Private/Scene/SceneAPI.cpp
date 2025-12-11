//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/SceneAPI.h"

#include <iostream>

#include "Framework/SceneManager.h"
#include "Scene/JScene.h"

EditorSceneAPI::EditorSceneAPI(EngineContext &ctx, SceneManager &scene):
m_Context(ctx),
m_SceneManager(scene)
{
}

std::vector<FEditorActorSnapshot> EditorSceneAPI::BuildHierarchySnapshot() const
{
    std::vector<FEditorActorSnapshot> result;

    JScene* scene = m_SceneManager.GetActiveScene();
    if (!scene)
        return result;

    const auto& actors = m_SceneManager.ListAllActors();
    result.reserve(actors.size());

    for (auto* actor : actors)
    {
        if (!actor) continue;

        FEditorActorSnapshot info{};
        info.id = actor->GetRuntimeID();
        info.parentID = actor->GetParentActor()
                            ? actor->GetParentActor()->GetRuntimeID()
                            : 0;
        info.name = actor->GetName();
        info.hasChildren = !actor->GetChildActors().empty();
        info.isSelected  = false; // will be filled by EditorCore

        result.push_back(std::move(info));
    }

    return result;
}

void EditorSceneAPI::SetSelectedActors(const std::vector<ActorID> &ids)
{
    m_SelectedActors = ids;
    // Later: propagate to renderer for outlines, etc.
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

bool EditorSceneAPI::Raycast(const FRay& ray, FRaycastHit& outHit) // TODO: Should detect based on AABB or mesh for future
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

    bool found = false;
    float bestDistance = std::numeric_limits<float>::max();

    for (JActor* actor : actors)
    {
        if (!actor)
            continue;

        if (!actor->IsVisible()) continue;

        // Actor world position
        const FVector3 actorPos = actor->GetActorLocation();

        // Vector from ray origin to actor
        const FVector3 toActor = actorPos - ray.Origin;

        // Project onto ray dir to get the closest point parameter t
        const float t = FMath::Dot(toActor, ray.Direction);
        if (t < minDistance)
            continue; // behind the ray or too close

        const FVector3 closestPoint = ray.Origin + ray.Direction * t;

        // Distance from actor center to ray
        const float distSq = (actorPos - closestPoint).LengthSquared();
        if (distSq > pickRadiusSq)
            continue; // too far from ray

        if (!actor->GetRootComponent())
        {
            std::cout << "Actor " << actor->GetName() << " has NO root component\n";
        }
        else
        {
            auto rootPos = actor->GetRootComponent()->GetWorldPosition();
            std::cout << "Actor " << actor->GetName()
                      << " rootPos=(" << rootPos.x << "," << rootPos.y << "," << rootPos.z << ")\n";
        }

        if (distSq <= pickRadiusSq)
        {
            std::cout << "Candidate: " << actor->GetRuntimeID()
                      << " name=" << actor->GetName()
                      << " pos=("
                      << actorPos.x << ", "
                      << actorPos.y << ", "
                      << actorPos.z << ")"
                      << "  t=" << t
                      << "  distSq=" << distSq
                      << std::endl;
        }
        // Choose the closest valid hit (smallest t)
        if (!found || t < bestDistance)
        {
            found = true;
            bestDistance = t;

            outHit.bHit = true;
            outHit.ActorID  = static_cast<ActorID>(actor->GetRuntimeID());
            outHit.Distance = t;
            outHit.Position = closestPoint;
            // outHit.Normal   = ... // leave empty for now
        }
    }

    return found;
}