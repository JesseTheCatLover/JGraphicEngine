//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/JScene.h"

#include "Core/Serialization/JsonWriter.h"

#include "Rendering/RRenderProxies.h"

#include "Scene/JActor.h"

JScene::JScene(const std::string &name):
m_Name(name)
{
}

void JScene::Initialize()
{
    // Scene setup logic before BeginPlay
}

void JScene::BeginPlay()
{
    for (auto& actor : m_Actors)
        actor->BeginPlay();
}

void JScene::Tick(float deltaTime)
{
    // Tick all actors that are not pending destroy
    for (auto& actor : m_Actors)
    {
        if (!actor || actor->IsPendingDestroy())
            continue;

        actor->Tick(deltaTime);
    }

    // After ticking, clean up destroyed actors
    FlushDestroyedActors();
}

void JScene::EndPlay()
{
    for (auto& actor : m_Actors)
        actor->EndPlay();
}

void JScene::DestroyScene()
{
    for (auto& actor : m_Actors)
        actor->DestroyActor();

    FlushDestroyedActors(); // immediately process all pending
}

void JScene::FlushDestroyedActors()
{
    for (size_t i = 0; i < m_Actors.size(); )
    {
        JActor* actor = m_Actors[i].get();
        if (actor && actor->IsPendingDestroy())
        {
            actor->EndPlay();
            actor->ExecuteDestroy();

            RemoveActor(actor);  // updates map + vector indices
            // RemoveActor swaps and pops, so DO NOT increment i here
        }
        else
        {
            ++i;
        }
    }
}

void JScene::Serialize(JsonWriter &writer) const
{
    Super::Serialize(writer);

    writer.Write("name", m_Name);
    writer.Write("actor_count", static_cast<int>(m_Actors.size()));

    writer.BeginArray("actors");
    for (const auto& actor : m_Actors)
    {
        actor->Serialize(writer);
    }
    writer.EndArray();
}

void JScene::Deserialize(const class JsonReader &reader)
{
    Super::Deserialize(reader);

    m_Name = reader.Read("name", std::string("Unnamed"));

    m_Actors.clear();
    m_ActorsByID.clear();

    auto actorsArray = reader.GetArray("actors");
    for (const auto& actorReader : actorsArray)
    {
        auto actor = std::make_unique<JActor>();
        actor->SetRuntimeID(actorReader.Read("id", 0));
        actor->SetVectorIndex(actorReader.Read("vector_index", 0));
        actor->SetName(actorReader.Read("name", std::string("Actor")));

        // Setup position & rotation from root component
        FVector3 pos = actorReader.ReadVector3("position", FVector3{});
        FRotator rot = actorReader.ReadRotator( "rotation", FRotator{});

        actor->SetActorPosition(pos);
        actor->SetActorRotation(rot);

        // Add actor to the scene and ID map
        AddActorToList(std::move(actor));
    }
}

void JScene::SetName(const std::string &name)
{
    m_Name = name;
    m_bIsDirty = true; // mark cache stale
}

void JScene::AddActorToList(std::unique_ptr<JActor> actor)
{
    actor->SetVectorIndex(m_Actors.size()); // track index
    actor->m_OwningScene = this; // takes ownership
    m_ActorsByID[actor->GetRuntimeID()] = actor.get();
    m_Actors.push_back(std::move(actor));
}

JActor* JScene::FindActorByID(uint64_t id)
{
    auto it = m_ActorsByID.find(id);
    return (it != m_ActorsByID.end()) ? it->second : nullptr;
}

bool JScene::RemoveActor(JActor *actorPtr)
{
    if(!actorPtr) return false;
    return RemoveActor(actorPtr->GetRuntimeID());
}

bool JScene::RemoveActor(uint64_t id)
{
    auto it = m_ActorsByID.find(id);
    if(it == m_ActorsByID.end())
        return false;

    JActor* actorPtr = it->second;
    size_t idx = actorPtr->GetVectorIndex();

    // Swap with last element and pop back
    if(idx != m_Actors.size() - 1)
    {
        std::swap(m_Actors[idx], m_Actors.back());
        m_Actors[idx]->SetVectorIndex(idx); // update swapped actor index
    }
    m_Actors.pop_back();
    m_ActorsByID.erase(it);

    m_bIsDirty = true; // mark cache stale
    return true;
}

void JScene::GatherRenderables(IRenderSubmission &submission, const FRenderContext &baseCtx) const
{
    for (const auto& actor : m_Actors)
    {
        if (!actor) continue;

        FRenderContext ctx = baseCtx;
        // e.g. ctx.layer override for special actor, etc.

        actor->GatherRenderables(submission, ctx);
    }
}

JCameraComponent* JScene::GetCameraComponent() const
{
    for (const auto& actor : m_Actors)
    {
        if (!actor) continue;

        if (actor->GetCameraComponent())
            return actor->GetCameraComponent();
    }
    return nullptr;
}
