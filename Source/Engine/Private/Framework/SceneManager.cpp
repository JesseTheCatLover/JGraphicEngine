#include "Framework/SceneManager.h"

#include <fstream>

#include "Core/EngineGlobals.h"
#include "Core/FObjectInitializer.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Core/Serialization/SerializationSubsystem.h"
#include "Scene/JActor.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"
#include "Core/Serialization/SerializeUtilities.h"

namespace
{
    static bool ResolveProjectScenePath(const std::string& sceneVirtualPath, std::string& outPhysicalPath)
    {
        outPhysicalPath.clear();

        if (!GEngine)
            return false;

        if (sceneVirtualPath.empty())
            return false;

        return GEngine->GetVirtualPathMounter().ResolveVirtualToPhysical(sceneVirtualPath, outPhysicalPath);
    }
}

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
    outInfo.sceneName = scene->GetObjectName();
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

        // Add root component explicitly (because root is not in m_SceneComponents)
        if (JSceneComponent* root = actor->GetRootComponent())
            outInfo.objects.push_back(root);

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
        if (auto* actor = dynamic_cast<JActor*>(obj))
            scene.TakeActorOwnershipFromLoad(actor);
    }

    // 2) Restore components (scene + logic). Scene components first.
    for (const FSceneObjectRelation& rel : loadResult.relations)
    {
        if (!rel.object) continue;

        // Scene component
        if (auto* sc = dynamic_cast<JSceneComponent*>(rel.object))
        {
            if (rel.ownerActorUUID.empty()) continue;

            auto itOwner = loadResult.uuidMap.find(rel.ownerActorUUID);
            if (itOwner == loadResult.uuidMap.end()) continue;

            auto* ownerActor = dynamic_cast<JActor*>(itOwner->second);
            if (!ownerActor) continue;

            JSceneComponent* parentComp = nullptr;
            if (!rel.parentComponentUUID.empty())
            {
                auto itParent = loadResult.uuidMap.find(rel.parentComponentUUID);
                if (itParent != loadResult.uuidMap.end())
                    parentComp = dynamic_cast<JSceneComponent*>(itParent->second);
            }

            // Own + register exactly once
            ownerActor->TakeComponentOwnershipFromLoad(sc, parentComp);
            continue;
        }

        // Actor component (logic)
        if (auto* ac = dynamic_cast<JActorComponent*>(rel.object))
        {
            if (rel.ownerActorUUID.empty()) continue;

            auto itOwner = loadResult.uuidMap.find(rel.ownerActorUUID);
            if (itOwner == loadResult.uuidMap.end()) continue;

            auto* ownerActor = dynamic_cast<JActor*>(itOwner->second);
            if (!ownerActor) continue;

            ownerActor->TakeComponentOwnershipFromLoad(ac, nullptr);
            continue;
        }
    }

    // 3) Fix root components from explicit rootComponentUUID
    for (const FSceneObjectRelation& rel : loadResult.relations)
    {
        auto* actor = dynamic_cast<JActor*>(rel.object);
        if (!actor) continue;

        if (rel.rootComponentUUID.empty()) continue;

        auto itRootObj = loadResult.uuidMap.find(rel.rootComponentUUID);
        if (itRootObj == loadResult.uuidMap.end()) continue;

        auto* rootComp = dynamic_cast<JSceneComponent*>(itRootObj->second);
        if (!rootComp) continue;

        actor->m_RootComponent = rootComp;

        // Root should not have a parent component (actor root semantics)
        rootComp->AttachToComponent(nullptr);
    }

    // 4) Restore actor hierarchy (now root pointers are correct)
    for (const FSceneObjectRelation& rel : loadResult.relations)
    {
        auto* actor = dynamic_cast<JActor*>(rel.object);
        if (!actor) continue;

        if (rel.parentActorUUID.empty())
        {
            actor->DetachFromParentActor(false);
            continue;
        }

        auto itParent = loadResult.uuidMap.find(rel.parentActorUUID);
        if (itParent == loadResult.uuidMap.end()) continue;

        auto* parentActor = dynamic_cast<JActor*>(itParent->second);
        if (!parentActor) continue;

        actor->AttachToActor(parentActor, false);
    }

    // 5) PostLoad last: graph is fully wired
    for (JCoreObject* obj : loadResult.objects)
        if (obj) obj->PostLoad();
}

bool SceneManager::CreateSceneFile(const std::string& sceneName,
                                   const std::string& sceneVirtualPath,
                                   bool bOverwrite) const
{
    std::string scenePath;
    if (!ResolveProjectScenePath(sceneVirtualPath, scenePath))
        return false;

    UFileSystem::CreateDirectory(UPath::GetParent(scenePath));

    if (UFileSystem::FileExists(scenePath) && !bOverwrite)
        return false;

    const FObjectInitializer init = FObjectInitializer::ForSceneRoot(sceneName);

    TUniquePtr<JScene> scene;
    {
        FObjectInitTLS::FScope scope(init);
        scene = MakeUnique<JScene>(); // ctor consumes TLS
    }

    // Set the path so the new scene inherently knows where it lives
    scene->SetVirtualFilePath(sceneVirtualPath);

    return SaveSceneFile(scene.get(), sceneVirtualPath);
}

JScene* SceneManager::LoadSceneFile(const std::string& sceneVirtualPath)
{
    std::string scenePath;
    if (!ResolveProjectScenePath(sceneVirtualPath, scenePath))
        return nullptr;

    if (!UFileSystem::FileExists(scenePath))
        return nullptr;

    FSceneLoadResult loadResult;
    if (!SerializationSubsystem::Get().LoadScene(scenePath, loadResult))
        return nullptr;

    const std::string sceneName = loadResult.sceneName.empty() ? "UnnamedScene" : loadResult.sceneName;

    FObjectInitializer init{};
    init.Name   = sceneName;
    init.Scene  = nullptr;
    init.Owner  = nullptr;
    init.bIsCDO = false;

    TUniquePtr<JScene> newScene;
    {
        FObjectInitTLS::FScope scope(init);
        newScene = MakeUnique<JScene>();
    }

    // Inject path into the newly loaded scene
    newScene->SetVirtualFilePath(sceneVirtualPath);

    ApplyLoadedResultToScene(loadResult, *newScene);

    if (OnSceneLoaded)
        OnSceneLoaded(newScene.get());

    m_ActiveScene = TakeUniqueOwnership(newScene);
    return m_ActiveScene.get();
}

bool SceneManager::SaveSceneFile(const JScene* scene, const std::string& sceneVirtualPath) const
{
    if (!scene)
        return false;

    if (!scene->m_bIsDirty)
        return true; // nothing to save

    std::string scenePath;
    if (!ResolveProjectScenePath(sceneVirtualPath, scenePath))
        return false;

    UFileSystem::CreateDirectory(UPath::GetParent(scenePath));

    FSceneSaveInfo info;
    BuildSaveInfoFromScene(scene, info);

    if (!SerializationSubsystem::Get().SaveScene(info, scenePath))
        return false;

    if (OnSceneSaved)
        OnSceneSaved(scene);

    return true;
}

bool SceneManager::SaveActiveScene() const
{
    if (!m_ActiveScene) return false;

    const std::string& currentPath = m_ActiveScene->GetVirtualFilePath();

    if (currentPath.empty())
    {
        // we might want to trigger a log warning or an editor popup here later
        return false;
    }

    return SaveSceneFile(m_ActiveScene.get(), currentPath);
}

bool SceneManager::SaveActiveSceneAs(const std::string &destinationVirtualPath, const std::string &sceneName)
{
    if (!m_ActiveScene) return false;
    if (destinationVirtualPath.empty()) return false;

    const std::string fullPath = destinationVirtualPath + "/" + sceneName + ".jscene";

    // Update the internal tracker to the new destination path
    m_ActiveScene->SetVirtualFilePath(fullPath);

    // Push changes to disk at the newly assigned path
    return SaveSceneFile(m_ActiveScene.get(), fullPath);
}

bool SceneManager::RenameScene(JScene *scene, const std::string &newName)
{
    if (!scene || newName.empty()) return false;
    if (scene->GetObjectName() == newName) return false; // Name is already the same

    const std::string& oldVirtualPath = scene->GetVirtualFilePath();

    // If the scene has a file on disk, rename the physical file too
    if (!oldVirtualPath.empty())
    {
        std::string oldPhysicalPath;
        if (!ResolveProjectScenePath(oldVirtualPath, oldPhysicalPath))
            return false; // Failed to resolve current path

        // Extract the directory from the virtual path to construct the new path
        // e.g., "/Project/Scenes/OldName.jscene" -> "/Project/Scenes"
        std::string parentDir = UPath::GetParent(oldVirtualPath);

        // Append the new name and extension
        std::string newVirtualPath = parentDir + "/" + newName + ".jscene";

        std::string newPhysicalPath;
        if (!ResolveProjectScenePath(newVirtualPath, newPhysicalPath))
            return false;

        // Check if a file with the new name already exists to avoid overwriting
        if (UFileSystem::FileExists(newPhysicalPath))
            return false;

        // Perform the file rename on disk using standard C++17 filesystem
        try
        {
            std::filesystem::rename(oldPhysicalPath, newPhysicalPath);
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            // Rename failed (e.g., file locked by another program, permission denied)
            return false;
        }

        // Update the scene's internal path tracker to match the new file location
        scene->SetVirtualFilePath(newVirtualPath);
    }

    // Update the object name in memory
    scene->SetObjectName(newName);

    // Fire the callback for the editor/UI systems
    if (OnSceneRenamed)
        OnSceneRenamed(scene, newName);

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