#include "Framework/SceneManager.h"

#include <fstream>

#include "Core/FObjectInitializer.h"
#include "Core/Serialization/SerializationSubsystem.h"
#include "Scene/JActor.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"
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

    // ----------------- Metadata -----------------
    if (!scene)
        return;

    // Name & actor count
    outInfo.sceneName = scene->GetName();
    outInfo.actorCount = static_cast<unsigned int>(scene->m_Actors.size());
    outInfo.thumbnail = "thumbnail.png";

    // Timestamp
    auto now       = std::chrono::system_clock::now();
    std::time_t ts = std::chrono::system_clock::to_time_t(now);

    std::tm timeInfo{};
#ifdef _WIN32
    localtime_s(&timeInfo, &ts);
#else
    localtime_r(&ts, &timeInfo);
#endif

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    outInfo.lastModified = std::string(buffer);

    // ----------------- Objects -----------------
    auto actors = scene->ListAllActors();

    for (JActor* actor : actors)
    {
        if (!actor) continue;

        // Add actor itself
        outInfo.objects.push_back(actor);

        // Actor components
        auto actorComps = actor->ListActorComponentsRaw();
        for (JActorComponent* comp : actorComps)
        {
            if (!comp) continue;
            outInfo.objects.push_back(comp);
        }

        // Scene components
        auto sceneComps = actor->ListSceneComponentsRaw();
        for (JSceneComponent* comp : sceneComps)
        {
            if (!comp) continue;
            outInfo.objects.push_back(comp);
        }
    }
}

void SceneManager::ApplyLoadedResultToScene(const FSceneLoadResult& loadResult, JScene& scene)
{
    // 1) Scene takes ownership of all actors
    for (JCoreObject* obj : loadResult.objects)
    {
        if (!obj) continue;
        if (auto* actor = dynamic_cast<JActor*>(obj))
        {
            scene.TakeActorOwnershipFromLoad(actor);
        }
    }

    // 2) Restore actor hierarchy
    for (const FSceneObjectRelation& rel : loadResult.relations)
    {
        if (!rel.object)
            continue;

        auto* actor = dynamic_cast<JActor*>(rel.object);
        if (!actor)
            continue; // this relation isn't for an actor

        if (rel.parentActorUUID.empty())
        {
            // Root actor; ensure no parent
            actor->DetachFromParentActor();
            continue;
        }

        // Find parent actor by UUID
        auto itParent = loadResult.uuidMap.find(rel.parentActorUUID);
        if (itParent == loadResult.uuidMap.end())
            continue;

        auto* parentActor = dynamic_cast<JActor*>(itParent->second);
        if (!parentActor)
            continue;

        actor->AttachToActor(parentActor);
    }

    // 3) Restore components (logic + scene) (Note: SCENE FIRST, then pure logic)
    for (const FSceneObjectRelation& rel : loadResult.relations)
    {
        if (!rel.object)
            continue;

        // -------- Scene components (transform/render) --------
        if (auto* sceneComp = dynamic_cast<JSceneComponent*>(rel.object))
        {
            if (rel.ownerActorUUID.empty())
                continue;

            auto itOwner = loadResult.uuidMap.find(rel.ownerActorUUID);
            if (itOwner == loadResult.uuidMap.end())
                continue;

            auto* ownerActor = dynamic_cast<JActor*>(itOwner->second);
            if (!ownerActor)
                continue;

            JSceneComponent* parentComp = nullptr;
            if (!rel.parentComponentUUID.empty())
            {
                auto itParentComp = loadResult.uuidMap.find(rel.parentComponentUUID);
                if (itParentComp != loadResult.uuidMap.end())
                    parentComp = dynamic_cast<JSceneComponent*>(itParentComp->second);
            }

            ownerActor->AttachSceneComponentFromLoad(sceneComp, parentComp);
            continue;  // IMPORTANT: don't also treat it as logic
        }

        // -------- Actor components (logic) --------
        if (auto* logic = dynamic_cast<JActorComponent*>(rel.object))
        {
            if (rel.ownerActorUUID.empty())
                continue;

            auto itOwner = loadResult.uuidMap.find(rel.ownerActorUUID);
            if (itOwner == loadResult.uuidMap.end())
                continue;

            auto* ownerActor = dynamic_cast<JActor*>(itOwner->second);
            if (!ownerActor)
                continue;

            ownerActor->AttachActorComponentFromLoad(logic);
            //continue; // done with this relation
        }
    }

    // 4) Fix up root components from explicit rootComponentUUID
    for (const FSceneObjectRelation& rel : loadResult.relations)
    {
        auto* actor = dynamic_cast<JActor*>(rel.object);
        if (!actor)
            continue;

        if (rel.rootComponentUUID.empty())
            continue; // this actor didn't serialize a root

        // Look up the root component object via the global uuidMap
        auto itRootObj = loadResult.uuidMap.find(rel.rootComponentUUID);
        if (itRootObj == loadResult.uuidMap.end())
            continue;

        auto* rootComp = dynamic_cast<JSceneComponent*>(itRootObj->second);
        if (!rootComp)
            continue;

        // Find the shared_ptr that owns this raw pointer inside this actor
        // SceneManager is a friend of JActor, so this is allowed.
        for (auto& compPtr : actor->m_SceneComponents)
        {
            if (compPtr.get() == rootComp)
            {
                actor->m_RootComponent = compPtr;
                break;
            }
        }
    }

    // 5) Now that the graph is fully wired, call PostLoad for every object
    for (JCoreObject* obj : loadResult.objects)
    {
        if (obj)
            obj->PostLoad();
    }
}

bool SceneManager::CreateSceneFile(const std::string& name,
                                  const std::string& filename,
                                  bool bOverwrite) const
{
    std::string scenePath = UPath::ResolvePath(UPath::Join("Assets", "Scenes", filename + ".jscene"));

    if (UFileSystem::FileExists(scenePath) && !bOverwrite)
        return false;

    FObjectInitializer Init = FObjectInitializer::ForSceneRoot(name);

    // Create by typename (or by typeid(JScene))
    JCoreObject* obj = RETypeRegistry::Get().CreateInstanceByTypeName("JScene", Init);
    auto* scene = dynamic_cast<JScene*>(obj);
    if (!scene)
        return false;

    return SaveSceneFile(scene, filename);
}
JScene* SceneManager::LoadSceneFile(const std::string& filename)
{
    std::string scenePath = UPath::ResolvePath(UPath::Join("Assets", "Scenes", filename + ".jscene")).string();

    if (!UFileSystem::FileExists(scenePath))
        return nullptr;

    FSceneLoadResult loadResult;
    if (!SerializationSubsystem::Get().LoadScene(scenePath, loadResult))
        return nullptr;

    std::string sceneName = loadResult.sceneName.empty()
                          ? std::string("UnnamedScene")
                          : loadResult.sceneName;

    FObjectInitializer Init{};
    Init.Name  = sceneName;
    Init.Scene = nullptr; // this is the scene itself
    Init.Owner = nullptr;

    auto newScene = TUniquePtr<JScene>(new JScene(Init));

    ApplyLoadedResultToScene(loadResult, *newScene);

    if (OnSceneLoaded)
        OnSceneLoaded(newScene.get());

    m_ActiveScene = TakeUniqueOwnership(newScene);
    return m_ActiveScene.get();
}

bool SceneManager::SaveSceneFile(const JScene *scene, const std::string &filename) const
{
    if (!scene)
        return false;

    if (!scene->m_bIsDirty)
        return true; // nothing to save

    std::string scenePath = UPath::ResolvePath(UPath::Join("Assets", "Scenes", filename + ".jscene")).string();

    FSceneSaveInfo info;
    BuildSaveInfoFromScene(scene, info);

    if (!SerializationSubsystem::Get().SaveScene(info, scenePath))
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
