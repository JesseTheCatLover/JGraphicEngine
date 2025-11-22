//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <vector>

class JSceneComponent;
class JActorComponent;
class JActor;
class JCoreObject;

struct FSceneSaveInfo
{
    std::vector<JCoreObject*> objects;  // all actors + components
    std::vector<JActor*> rootActors;
    std::unordered_map<JActor*, std::vector<JActorComponent*>> actorComponents; // logic
    std::unordered_map<JActor*, std::vector<JSceneComponent*>> sceneComponents; // transform/render

    /** @brief Name of the scene. */
    std::string sceneName;

    /** @brief Number of actors currently in the scene. */
    unsigned int actorCount;

    /** @brief Path or identifier for a thumbnail image representing the scene. */
    std::string thumbnail;

    /** @brief Timestamp of the last modification of the scene file (human-readable). */
    std::string lastModified;
};

struct FSceneLoadResult
{
    std::vector<JCoreObject*> objects;
    std::vector<JActor*> rootActors;
    std::unordered_map<JActor*, std::vector<JActorComponent*>> actorComponents;
    std::unordered_map<JActor*, std::vector<JSceneComponent*>> sceneComponents;

    /** @brief Name of the scene. */
    std::string sceneName;

    /** @brief Number of actors currently in the scene. */
    unsigned int actorCount;

    /** @brief Path or identifier for a thumbnail image representing the scene. */
    std::string thumbnail;

    /** @brief Timestamp of the last modification of the scene file (human-readable). */
    std::string lastModified;
};

class JSerializeManager
{
public:
    static JSerializeManager& Get()
    {
        static JSerializeManager instance;
        return instance;
    }

    bool SaveScene(const FSceneSaveInfo& info, const std::string& filePath);
    bool LoadScene(const std::string& filePath, FSceneLoadResult& outResult);

private:
    // helper to create object by type name from registry
    JCoreObject* CreateObjectByTypeName(const char* typeName);
};
