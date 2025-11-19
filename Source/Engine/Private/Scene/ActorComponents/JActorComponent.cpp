//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/ActorComponents/JActorComponent.h"
#include "Core/Serialization/JsonWriter.h"

#include "Scene/JActor.h"

void JActorComponent::Initialize()
{
}

void JActorComponent::BeginPlay()
{
}

void JActorComponent::EndPlay()
{
}

void JActorComponent::OnDestroy()
{
}

void JActorComponent::OnAttachment()
{
}

void JActorComponent::Tick(float deltaTime)
{
}

bool JActorComponent::DestroyComponent()
{
    if (m_bPendingDestroy)
        return false;

    m_bPendingDestroy = true;
    return true;
}

void JActorComponent::Serialize(JsonWriter &writer) const
{
    writer.Write("type", GetClassTypeName());
    writer.Write("id", GetRuntimeID());
}

void JActorComponent::Deserialize(const JsonReader &reader)
{
}
