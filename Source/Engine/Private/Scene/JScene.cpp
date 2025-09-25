//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JScene.h"
#include "Scene/JActor.h"

JScene::JScene(const std::string &name):
m_Name(name), m_NextActorID(1)
{
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

    // Editor callback
    if(OnActorRemoved)
        OnActorRemoved(actorPtr);

    return true;
}

void JScene::AddActorToList(std::unique_ptr<JActor> actor)
{
    actor->m_VectorIndex = m_Actors.size(); // track index
    m_ActorsByID[actor->ID] = actor.get();
    m_Actors.push_back(std::move(actor));
    // Editor callback
    if(OnActorAdded)
        OnActorAdded(m_Actors.back().get());
}

template<typename T, typename... Args>
T *JScene::SpawnActor(Args &&... args)
{
    static_assert(std::is_base_of<JActor, T>::value, "T must derive from JActor");

    // Create actor of type T with forwarded constructor arguments
    auto actor = std::make_unique<T>(std::forward<Args>(args)...);
    actor->id = m_NextActorID++; // assign unique ID

    T* ptr = actor.get();
    AddActorToList(std::move(actor));
    return ptr;
}

template<typename T>
T * JScene::FindActorByID(unsigned int id)
{
    static_assert(std::is_base_of<JActor, T>::value, "T must derive from JActor");

    auto it = m_ActorsByID.find(id);
    if (it == m_ActorsByID.end()) return nullptr;

    return static_cast<T*>(it->second); // assume you know the type
}

template<typename T>
T * JScene::FindActorOfType()
{
    static_assert(std::is_base_of<JActor, T>::value, "T must derive from JActor");

    for (auto& actor : m_Actors)
    {
        if (T* casted = dynamic_cast<T*>(actor.get()))
            return casted; // return first match
    }
    return nullptr; // no match found
}

template<typename T>
std::vector<T *> JScene::FindActorsOfType()
{
    static_assert(std::is_base_of<JActor, T>::value, "T must derive from JActor");

    std::vector<T*> results;
    for (auto& actor : m_Actors)
    {
        if (T* casted = dynamic_cast<T*>(actor.get()))
            results.push_back(casted);
    }
    return results;
}
