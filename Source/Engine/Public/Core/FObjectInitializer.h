//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

class JActor;
class JScene;
class JCoreObject;

struct FObjectInitializer
{
    JScene* Scene = nullptr;
    JCoreObject* Owner = nullptr;
    std::string Name;
    bool bIsCDO = false;

    // Set by JCoreObject base ctor automatically (via TLS)
    mutable JCoreObject* ConstructingObject = nullptr;

    static FObjectInitializer ForSceneRoot(const std::string& name)
    {
        FObjectInitializer I{};
        I.Name = name;
        return I;
    }

    static FObjectInitializer ForObject(JScene* scene, JCoreObject* owner, const std::string& name)
    {
        FObjectInitializer I{};
        I.Scene = scene;
        I.Owner = owner;
        I.Name = name;
        return I;
    }

    static FObjectInitializer ForCDO()
    {
        FObjectInitializer I{};
        I.bIsCDO = true;
        return I;
    }
};
