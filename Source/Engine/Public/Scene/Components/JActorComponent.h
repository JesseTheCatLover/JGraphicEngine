//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include "Core/JCoreObject.h"

class JActor;

/**
 * @class JActorComponent
 * @brief Base class for all ECS components.
 *
 * Components derive from JCoreObject to get unique IDs and serialization.
 * They are attached to actors (or entities) to provide behavior/data.
 */
class JActorComponent : public JCoreObject
{
    DECLARE_JOBJECT(JActorComponent)

    friend class JActor;
    friend class JSceneComponent;

private:
    std::string m_Name;
    JActor* m_OwnerActor = nullptr; ///< Actor this component is attached to

public:
    JActorComponent() = default;
    virtual ~JActorComponent() = default;

    /**
     * @brief Serialize this component into JSON.
     * Calls SerializeProperties() to allow subclasses to write their own fields.
     */
    void Serialize(JsonWriter& Writer) const override;

    /**
     * @brief Deserialize this component from JSON.
     * Calls DeserializeProperties() to allow subclasses to read their own fields.
     */
    void Deserialize(const JsonReader& Reader) override;

    /**
     * @brief Optional per-frame update for the component.
     * Override to add runtime logic.
     * @param DeltaTime Time since last tick in seconds.
     */
    virtual void Tick(float DeltaTime);

    /** @brief Get the owning actor. */
    JActor* GetOwnerActor() const { return m_OwnerActor; }

    /** @brief Get the name of the component */
    std::string GetName() const { return m_Name; }

    /** @brief Set the name of the component */
    void SetName(const std::string& InName) { m_Name = InName; }

protected:
    /** @brief Set the owning actor for this component. */
    void SetOwnerActor (JActor* InActor) { m_OwnerActor = InActor; }

    /** @brief Called when the component is attached to a parent actor. */
    virtual void OnAttachment();

    /** @brief Called after OnAttachment() call. */
    virtual void Initialize();

    /**
    * @brief Called when the component is starting to "play" in the scene.
    *
    *  This is part of the component's lifecycle. Called by the owning actor
    *  when the scene begins. Subclasses can override this to implement
    *  initialization or runtime behavior that depends on the actor being active.
    */
    void BeginPlay();

    /**
     * @brief Called when the component is ending its "play" in the scene.
     *
     * This is part of the component's lifecycle. Called by the owning actor
     * when the actor or scene is shutting down. Subclasses can override
     * this to implement cleanup or finalization logic.
     */
    void EndPlay();

    /**
     * @brief Serialize subclass-specific properties into JSON.
     * Called by Serialize().
     */
    virtual void SerializeProperties(JsonWriter& Writer) const = 0;

    /**
     * @brief Deserialize subclass-specific properties from JSON.
     * Called by Deserialize().
     */
    virtual void DeserializeProperties(const JsonReader& Reader) = 0;
};
