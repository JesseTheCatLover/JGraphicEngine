#include "Framework/SceneManager.h"

JScene* JSceneManager::CreateScene(const std::string& name)
{
    auto scene = std::make_unique<JScene>(name);
    JScene* ptr = scene.get();
    m_Scenes[name] = std::move(scene);
    return ptr;
}

void JSceneManager::SetActiveScene(const std::string& name)
{
    auto it = m_Scenes.find(name);
    if(it != m_Scenes.end())
        m_ActiveScene = it->second.get();
}

JActor* JSceneManager::FindActorByID(unsigned int id)
{
    if(!m_ActiveScene) return nullptr;
    return m_ActiveScene->FindActorByID(id);
}

bool JSceneManager::RemoveActor(JActor *actorPtr)
{
    if (!m_ActiveScene || !actorPtr) return false;
    bool removed = m_ActiveScene->RemoveActor(actorPtr);
    if (removed && OnActorRemoved) OnActorRemoved(actorPtr);
    return removed;
}

bool JSceneManager::RemoveActor(unsigned int id)
{
    if (!m_ActiveScene) return false;
    JActor* actor = m_ActiveScene->FindActorByID(id);
    if (!actor) return false;
    return RemoveActor(actor);
}

void JSceneManager::Update(float deltaTime)
{
    if(m_ActiveScene)
        m_ActiveScene->UpdateActors(deltaTime);
}
