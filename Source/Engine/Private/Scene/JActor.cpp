// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/JActor.h"
#include "Scene/SceneComponents/JSceneComponent.h"

#include "Core/JEngine.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Scene/SceneComponents/JModelComponent.h"

JActor::JActor() : m_VectorIndex(0)
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
        m_RootComponent = MakeShared<JSceneComponent>();
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

void JActor::Tick(float deltaTime)
{
    // Tick all components
    for (auto& comp : m_ActorComponents)
        comp->Tick(deltaTime);
    for (auto& comp : m_SceneComponents)
        comp->Tick(deltaTime);
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

void JActor::GatherRenderables(IRenderSubmission &submission, const FRenderContext &ctx) const // TODO: This is temp
{
    // Actor-level culling / conditions can go here later:
    // if (!bIsVisible || IsTooFarFromCamera(...)) return;

    for (auto& comp : m_SceneComponents)
    {
        if (auto* renderable = dynamic_cast<JRenderableComponent*>(comp.get()))
        {
            // Let the component turn itself into proxies / draw commands
            renderable->GatherProxies(submission, ctx);
        }
    }
}

void JActor::Serialize(JsonWriter& writer) const
{
    writer.BeginObject();
    writer.Write("name", m_Name);

    // Serialize components
    writer.BeginArray("components");
    for (auto& comp : m_ActorComponents)
    {
        comp->Serialize(writer);
    }
    writer.EndArray();

    // Serialize scene components
    writer.BeginArray("scene_components");
    for (auto& comp : m_SceneComponents)
    {
        comp->Serialize(writer);
    }
    writer.EndArray();

    writer.EndObject();
}

void JActor::Deserialize(const JsonReader& reader)
{
    m_Name = reader.Read<std::string>("name", "");

    // Scene components
    if (reader.Has("scene_components"))
    {
        auto sceneCompsReader = reader.GetArray("scene_components");
        for (const auto& compJson : sceneCompsReader)
        {
            std::string type = compJson.Read<std::string>("type", "");

            // Only handle JModelComponent for now
            if (type == "JModelComponent")
            {
                auto* comp = new JModelComponent();
                comp->SetOwnerActor(this);
                comp->Deserialize(compJson);
                m_SceneComponents.push_back(TSharedPtr<JSceneComponent>(comp));

                // Also assign to ModelComponent pointer for convenience
                ModelComponent = comp;
            }
        }
    }

    //  Skip actor components for now cause not needed
}
