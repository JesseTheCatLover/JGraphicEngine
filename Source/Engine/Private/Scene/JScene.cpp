//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/JScene.h"
#include "Scene/JActor.h"

using json = nlohmann::json;

JScene::JScene(const std::string &name):
m_Name(name), m_NextActorID(1)
{
}

void JScene::SetName(const std::string &name)
{
    m_Name = name;
    m_bIsDirty = true; // mark cache stale
}

void JScene::UpdateActors(float deltaTime)
{
    for (auto& actor : m_Actors)
    {
        //if (actor)
            //actor->Update(deltaTime); TODO: Implement Update for each JActor
    }
}

JActor* JScene::FindActorByID(unsigned int id)
{
    auto it = m_ActorsByID.find(id);
    return (it != m_ActorsByID.end()) ? it->second : nullptr;
}

bool JScene::RemoveActor(JActor *actorPtr)
{
    if(!actorPtr) return false;
    return RemoveActor(actorPtr->ID);
}

bool JScene::RemoveActor(unsigned int id)
{
    auto it = m_ActorsByID.find(id);
    if(it == m_ActorsByID.end())
        return false;

    JActor* actorPtr = it->second;
    size_t idx = actorPtr->m_VectorIndex;

    // Swap with last element and pop back
    if(idx != m_Actors.size() - 1)
    {
        std::swap(m_Actors[idx], m_Actors.back());
        m_Actors[idx]->m_VectorIndex = idx; // update swapped actor index
    }
    m_Actors.pop_back();
    m_ActorsByID.erase(it);

    m_bIsDirty = true; // mark cache stale
    return true;
}

nlohmann::json JScene::Serialize() const
{
    if (m_bIsDirty)
    {
        json j;
        j["name"] = m_Name;
        j["next_actor_id"] = m_NextActorID;
        j["actor_count"] = m_Actors.size();
        j["actors"] = json::array();

        for (const auto& actor : m_Actors)
        {
            json actorData;
            actorData["id"] = actor->ID;
            actorData["vector_index"] = actor->m_VectorIndex;
            actorData["name"] = actor->Name;
            actorData["position"] = {
                {"x", actor->Position.x},
                {"y", actor->Position.y},
                {"z", actor->Position.z},
            };
            actorData["rotation"] = {
                {"x", actor->Rotation.x},
                {"y", actor->Rotation.y},
                {"z", actor->Rotation.z},
            };

            j["actors"].push_back(actorData);
        }

        m_CachedJson = std::move(j);
        m_bIsDirty = false;
    }

    return m_CachedJson;
}

void JScene::Deserialize(const nlohmann::json &data)
{
    m_Name = data.value("name", "Unnamed");
    m_NextActorID = data.value("next_actor_id", 1);

    m_Actors.clear();
    m_ActorsByID.clear();

    if (data.contains("actors")) // Loading it
    {
        for (const auto& actorData : data["actors"])
        {
            // TODO: For now: just spawn generic JActor
            auto actor = std::make_unique<JActor>();
            actor->ID = actorData.value("id", m_NextActorID);
            actor->m_VectorIndex = actorData.value("vector_index", 0);
            actor->Position.x = actorData["position"].value("x", 0.0f);
            actor->Position.y = actorData["position"].value("y", 0.0f);
            actor->Position.z = actorData["position"].value("z", 0.0f);
            actor->Rotation.x = actorData["rotation"].value("x", 0.0f);
            actor->Rotation.y = actorData["rotation"].value("y", 0.0f);
            actor->Rotation.z = actorData["rotation"].value("z", 0.0f);

            AddActorToList(std::move(actor));
        }
    }
}

void JScene::AddActorToList(std::unique_ptr<JActor> actor)
{
    actor->m_VectorIndex = m_Actors.size(); // track index
    m_ActorsByID[actor->ID] = actor.get();
    m_Actors.push_back(std::move(actor));
}
