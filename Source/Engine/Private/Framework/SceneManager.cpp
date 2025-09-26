#include "Framework/SceneManager.h"

#include <fstream>
#include "nlohmann/json.hpp"

#include "Scene/JActor.h"

using json = nlohmann::json;

JActor* SceneManager::FindActorByID(unsigned int id) const
{
    if(!m_ActiveScene) return nullptr;
    return m_ActiveScene->FindActorByID(id);
}

bool SceneManager::RemoveActor(JActor *actorPtr)
{
    if (!m_ActiveScene || !actorPtr) return false;

    if (OnActorRemoving) OnActorRemoving(actorPtr);
    unsigned int id = actorPtr->ID;
    bool removed = m_ActiveScene->RemoveActor(actorPtr);
    if (removed && OnActorRemoved) OnActorRemoved(id);
    return removed;
}

bool SceneManager::RemoveActor(unsigned int id)
{
    if (!m_ActiveScene) return false;
    JActor* actor = m_ActiveScene->FindActorByID(id);
    if (!actor) return false;
    return RemoveActor(actor);
}

void SceneManager::Update(float deltaTime)
{
    if(m_ActiveScene)
        m_ActiveScene->UpdateActors(deltaTime);
}

bool SceneManager::CreateSceneFile(const std::string &name, const std::string &filename, bool bOverwrite) const
{
    if (std::filesystem::exists(std::string(ENGINE_DIRECTORY) + "/Assets/Scenes/" +
        filename + ".jscene") && !bOverwrite) return false;

    JScene scene(name); // Create a new empty scene object
    return SaveScene(&scene, filename); // Save and serialize it
}

JScene* SceneManager::LoadScene(const std::string &filename)
{
    std::ifstream file(std::string(ENGINE_DIRECTORY) + "/Assets/Scenes/" + filename + ".jscene");
    if (!file.is_open()) return nullptr;

    json j;
    file >> j;

    std::string name = j.value("name", "UnnamedScene");
    auto scene = std::make_unique<JScene>(name);
    scene->Deserialize(j);

    if (OnSceneLoaded) OnSceneLoaded(scene.get());
    m_ActiveScene = std::move(scene);
    // Ownership can be handled by smart pointer in manager or returned to caller
    return m_ActiveScene.get();
}

bool SceneManager::SaveScene(const JScene *scene, const std::string &filename) const
{
    if (!scene) return false;
    if (!scene->m_bIsDirty) return true; // nothing to do

    std::ofstream file(std::string(ENGINE_DIRECTORY) + "/Assets/Scenes/" + filename + ".jscene");
    if (!file.is_open()) return false;

    json j = scene->Serialize();

    // MetaData
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    j["meta"]["last_modified"] = std::string(std::ctime(&timestamp));

    file << j.dump(4);
    if (OnSceneSaved) OnSceneSaved(scene);
    return true;
}

bool SceneManager::RenameScene(JScene *scene, const std::string &newName)
{
    if (!scene || newName.empty()) return false;
    if (scene->m_Name == newName) return false;

    scene->SetName(newName);
    if (OnSceneRenamed) OnSceneRenamed(scene, newName);
    return true;
}

SceneMeta SceneManager::ReadSceneMeta(const std::string &filename)
{
    SceneMeta meta;
    std::ifstream file(filename);
    if (!file.is_open()) return meta;

    json j;
    file >> j;

    meta.name = j.value("name", "Unnamed");
    meta.actorCount = j.value("actor_count", 0);
    meta.lastModified = j["meta"].value("last_modified", "");
    meta.thumbnail = j["meta"].value("thumbnail", "");
    return meta;
}

std::vector<SceneMeta> SceneManager::ListScenesMeta(const std::string &directory)
{
    std::vector<SceneMeta> result;
    for (auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".jscene")
            result.push_back(ReadSceneMeta(entry.path().string()));
    }
    return result;
}

std::vector<std::string> SceneManager::ListAvailableSceneFiles(const std::string &directory)
{
    std::vector<std::string> files;
    for (auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".jscene")
            files.push_back(entry.path().filename().string());
    }
    return files;
}
