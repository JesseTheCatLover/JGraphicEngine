// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/JActor.h"
#include "Scene/Components/SceneComponents/JSceneComponent.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "glm/gtc/matrix_transform.hpp"

#include "Rendering/JModel.h"

#include "Scene/Components/SceneComponents/JModelComponent.h"

JActor::JActor()
{
    // Ensure root component exists
    SetupRootComponent();

    // Add default components for this actor
    ModelComponent = CreateDefaultComponent<JModelComponent>("Model");
    ModelComponent->AttachToComponent(GetRootComponent());
}

void JActor::SetupRootComponent()
{
    if (!m_RootComponent)
    {
        m_RootComponent = std::make_shared<JSceneComponent>();
        m_RootComponent->SetOwnerActor(this);
        m_RootComponent->SetName("RootComponent");
        m_SceneComponents.push_back(m_RootComponent);
    }
}

void JActor::Initialize()
{
    // Call Initialize on all components
    for (auto& comp : m_ActorComponents)
        comp->Initialize();
    for (auto& comp : m_SceneComponents)
        comp->Initialize();
}

void JActor::BeginPlay()
{
    // Call BeginPlay on all components
    for (auto& comp : m_ActorComponents)
        comp->BeginPlay();
    for (auto& comp : m_SceneComponents)
        comp->BeginPlay();
}

void JActor::Tick(float DeltaTime)
{
    // Tick all components
    for (auto& comp : m_ActorComponents)
        comp->Tick(DeltaTime);
    for (auto& comp : m_SceneComponents)
        comp->Tick(DeltaTime);
}

void JActor::EndPlay()
{
    // Call EndPlay on all components
    for (auto& comp : m_ActorComponents)
        comp->EndPlay();
    for (auto& comp : m_SceneComponents)
        comp->EndPlay();
}

void JActor::Destroy()
{
    m_ActorComponents.clear();
    m_SceneComponents.clear();
    m_RootComponent.reset();
}

void JActor::Draw(JShader& Shader) const
{
    for (auto& comp : m_SceneComponents)
    {
        // Assuming scene components have a Draw method
        if (comp->GetClassTypeName() == "JModelComponent")
        {
            if (auto* modelComp = dynamic_cast<JModelComponent*>(comp.get()))
                modelComp->Draw(Shader);
        }
    }
}

void JActor::Serialize(JsonWriter& Writer) const
{
    Writer.BeginObject();
    Writer.Write("name", m_Name);

    // Serialize components
    Writer.BeginArray("components");
    for (auto& comp : m_ActorComponents)
    {
        comp->Serialize(Writer);
    }
    Writer.EndArray();

    // Serialize scene components
    Writer.BeginArray("scene_components");
    for (auto& comp : m_SceneComponents)
    {
        comp->Serialize(Writer);
    }
    Writer.EndArray();

    Writer.EndObject();
}

void JActor::Deserialize(const JsonReader& Reader)
{
    m_Name = Reader.Read("name", "");

    // TODO: Deserialize components and scene components via reflection/factory
}
