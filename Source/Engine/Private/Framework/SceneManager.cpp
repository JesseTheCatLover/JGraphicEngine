#include "Framework/SceneManager.h"

#include <fstream>

#include "Core/Serialization/JSerializeManager.h"
#include "Scene/JActor.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPathFinder.h"
#include "Core/Serialization/SerializeUtilities.h"

JActor* SceneManager::FindActorByID(uint64_t id) const
{
    if(!m_ActiveScene) return nullptr;
    return m_ActiveScene->FindActorByID(id);
}

bool SceneManager::DestroyActor(JActor *actorPtr)
{
    if (!m_ActiveScene || !actorPtr)
        return false;

    if (OnActorRemoving)
        OnActorRemoving(actorPtr);

    // Only mark this actor as pending destroy.
    // JScene will clean it up in FlushDestroyedActors().
    return actorPtr->DestroyActor();
}

bool SceneManager::DestroyActor(uint64_t id)
{
    if (!m_ActiveScene)
        return false;

    JActor* actorPtr = m_ActiveScene->FindActorByID(id);
    return DestroyActor(actorPtr);
}

bool SceneManager::ImmediateDestroyActor(uint64_t id)
{
    if (!m_ActiveScene)
        return false;

    // First, request destruction (fires OnActorRemoving & pending flag).
    const bool requested = DestroyActor(id);
    if (!requested)
        return false; // no actor / already pending / no active scene

    // Now force immediate cleanup this frame.
    m_ActiveScene->FlushDestroyedActors();

    return true;
}

void SceneManager::Tick(float deltaTime)
{
    if(m_ActiveScene)
        m_ActiveScene->Tick(deltaTime);
}

void SceneManager::BuildSaveInfoFromScene(const JScene* scene, FSceneSaveInfo& outInfo) const
{
    outInfo.objects.clear();
    outInfo.rootActors.clear();
    outInfo.actorComponents.clear();

    // Meta data
    auto now = std::chrono::system_clock::now();
    std::time_t timestamp = std::chrono::system_clock::to_time_t(now);

    outInfo.sceneName = scene->GetName();
    outInfo.actorCount = scene->m_Actors.size();
    outInfo.thumbnail = "thumbnail.png";

    std::tm timeInfo{};
#ifdef _WIN32
    localtime_s(&timeInfo, &timestamp);
#else
    localtime_r(&timestamp, &timeInfo);
#endif

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    outInfo.lastModified =  std::string(buffer);


    if (!scene)
        return;

    auto actors = scene->GetAllActors();

    for (JActor* actor : actors)
    {
        if (!actor) continue;

        outInfo.objects.push_back(actor);

        if (actor->IsRootActor())
            outInfo.rootActors.push_back(actor);

        // Logic components
        auto actorComps = actor->GetActorComponentsRaw();
        auto& logicList = outInfo.actorComponents[actor];
        for (JActorComponent* comp : actorComps)
        {
            if (!comp) continue;
            outInfo.objects.push_back(comp);
            logicList.push_back(comp);
        }

        // Scene components
        auto sceneComps = actor->GetSceneComponentsRaw();
        auto& sceneList = outInfo.sceneComponents[actor];
        for (JSceneComponent* comp : sceneComps)
        {
            if (!comp) continue;
            outInfo.objects.push_back(comp);
            sceneList.push_back(comp);
        }
    }
}

void SceneManager::ApplyLoadedResultToScene(const FSceneLoadResult& loadResult, JScene& scene)
{
    //  Register all actors
    for (JCoreObject* obj : loadResult.objects)
    {
        if (!obj) continue;
        if (auto* actor = dynamic_cast<JActor*>(obj))
            scene.TakeActorOwnershipFromLoad(actor);
    }

    // TODO: Root actors handling


    // Attach actor components
    for (auto& [actor, comps] : loadResult.actorComponents)
    {
        if (!actor) continue;

        for (JActorComponent* comp : comps)
        {
            if (!comp) continue;
            actor->AttachActorComponentFromLoad(comp);
        }
    }

    // Attach scene components
    for (auto& [actor, comps] : loadResult.sceneComponents)
    {
        if (!actor) continue;

        for (JSceneComponent* comp : comps)
        {
            if (!comp) continue;

            // For now, attach everything to actor's root.
            // Later we can restore full component hierarchy using extra metadata.
            actor->AttachSceneComponentFromLoad(comp);
        }
    }
}

bool SceneManager::CreateSceneFile(const std::string &name, const std::string &filename, bool bOverwrite) const
{
    std::string scenePath = UPathFinder::ResolvePath(UPathFinder::Join("Assets", "Scenes", filename + ".jscene"));

    if (UFileSystem::FileExists(scenePath) && !bOverwrite)
        return false;

    JScene scene(name);
    return SaveSceneFile(&scene, filename);
}

JScene* SceneManager::LoadSceneFile(const std::string &filename)
{
    std::string scenePath = UPathFinder::ResolvePath(UPathFinder::Join("Assets", "Scenes", filename + ".jscene"));

    if (!UFileSystem::FileExists(scenePath))
        return nullptr;

    FSceneLoadResult loadResult;
    if (!JSerializeManager::Get().LoadScene(scenePath, loadResult))
        return nullptr;

    std::string sceneName = loadResult.sceneName.empty()
                          ? std::string("UnnamedScene")
                          : loadResult.sceneName;

    auto newScene = std::unique_ptr<JScene>(new JScene(sceneName));
    ApplyLoadedResultToScene(loadResult, *newScene);

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

    std::string scenePath = UPathFinder::ResolvePath(UPathFinder::Join("Assets", "Scenes", filename + ".jscene"));

    FSceneSaveInfo info;
    BuildSaveInfoFromScene(scene, info);

    if (!JSerializeManager::Get().SaveScene(info, scenePath))
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
    meta.actorCount = reader.Read<unsigned int>("actor_count", 0);

    if (reader.Has("meta"))
    {
        auto metaReader = reader.GetObject("meta");
        meta.thumbnail = metaReader.Read<std::string>("thumbnail", "");
        meta.lastModified = metaReader.Read<std::string>("last_modified", "");
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
