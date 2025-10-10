#include "Framework/SceneManager.h"

#include <fstream>
#include "nlohmann/json.hpp"
#include "Scene/JActor.h"

#include "Utilities/UFileSystem.h"
#include "Utilities/UPathFinder.h"
#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"

JActor* SceneManager::FindActorByID(unsigned int id) const
{
    if(!m_ActiveScene) return nullptr;
    return m_ActiveScene->FindActorByID(id);
}

bool SceneManager::RemoveActor(JActor *actorPtr)
{
    if (!m_ActiveScene || !actorPtr) return false;

    if (OnActorRemoving) OnActorRemoving(actorPtr);
    unsigned int id = actorPtr->GetID();
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
        m_ActiveScene->Tick(deltaTime);
}

bool SceneManager::CreateSceneFile(const std::string &name, const std::string &filename, bool bOverwrite) const
{
    std::string scenePath = UPathFinder::Join(ENGINE_DIRECTORY, "Assets", "Scenes", filename + ".jscene");

    if (UFileSystem::FileExists(scenePath) && !bOverwrite)
        return false;

    JScene scene(name);
    return SaveSceneFile(&scene, filename);
}

JScene* SceneManager::LoadSceneFile(const std::string &filename)
{
    std::string scenePath = UPathFinder::Join(ENGINE_DIRECTORY, "Assets", "Scenes", filename + ".jscene");

    if (!UFileSystem::FileExists(scenePath))
        return nullptr;

    JsonReader reader;
    if (!reader.LoadFromFile(scenePath))
        return nullptr;

    auto sceneName = reader.Read<std::string>("name", "UnnamedScene");

    auto newScene = std::unique_ptr<JScene>(new JScene(sceneName));
    newScene->Deserialize(reader);

    if (OnSceneLoaded)
        OnSceneLoaded(newScene.get());

    m_ActiveScene = std::move(newScene);
    return m_ActiveScene.get();
}

bool SceneManager::SaveSceneFile(const JScene *scene, const std::string &filename) const
{
    if (!scene)
        return false;

    if (!scene->m_bIsDirty)
        return true; // nothing to save

    std::string scenePath = UPathFinder::Join(ENGINE_DIRECTORY, "Assets", "Scenes", filename + ".jscene");

    JsonWriter writer;
    scene->Serialize(writer);

    // Meta data
    auto now = std::chrono::system_clock::now();
    std::time_t timestamp = std::chrono::system_clock::to_time_t(now);

    std::tm timeInfo{};
#ifdef _WIN32
    localtime_s(&timeInfo, &timestamp);
#else
    localtime_r(&timestamp, &timeInfo);
#endif

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    writer.BeginObject("meta");
    writer.Write("last_modified", std::string(buffer));
    writer.EndObject();

    if (!writer.SaveToFile(scenePath))
        return false;

    if (OnSceneSaved)
        OnSceneSaved(scene);

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

FSceneMeta SceneManager::ReadSceneMeta(const std::string &filename)
{
    FSceneMeta meta;

    JsonReader reader;
    if (!reader.LoadFromFile(filename))
        return meta;

    meta.name = reader.Read<std::string>("name", "Unnamed");
    meta.actorCount = reader.Read("actor_count", 0);

    if (reader.Has("meta"))
    {
        auto metaReader = reader.GetObject("meta");
        meta.lastModified = metaReader.Read<std::string>("last_modified", "");
        meta.thumbnail = metaReader.Read<std::string>("thumbnail", "");
    }

    return meta;
}

std::vector<FSceneMeta> SceneManager::ListScenesMeta(const std::string &directory)
{
    std::vector<FSceneMeta> result;

    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".jscene")
        {
            auto meta = ReadSceneMeta(entry.path().string());
            if (!meta.name.empty())
                result.push_back(meta);
        }
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
