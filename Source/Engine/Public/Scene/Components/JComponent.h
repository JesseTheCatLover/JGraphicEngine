//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/JCoreObject.h"

class JActor;

/**
 * @class JComponent
 * @brief Base class for all ECS components.
 *
 * Components derive from JCoreObject to get unique IDs and serialization.
 * They are attached to actors (or entities) to provide behavior/data.
 */
class JComponent : public JCoreObject
{
    DECLARE_JOBJECT(JComponent)

private:
    JActor* m_OwnerActor = nullptr; ///< Actor this component is attached to

    /** @brief Set the owning actor for this component. */
    void SetOwner (JActor* InActor) { m_OwnerActor = InActor; }

public:
    JComponent() = default;
    virtual ~JComponent() = default;

    /**
     * @brief Public API to attach this component to an actor.
     *
     * Sets the owning actor and calls OnAttachment() and Initialize() internally.
     * Subclasses should override OnAttachment()/Initialize() to handle custom initialization.
     *
     * @param owner Actor to attach to.
     */
    void SetupAttachment(JActor* owner);

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
    JActor* GetOwner() const { return Owner; }

protected:
    /** @brief Called when the component is attached to a parent actor. */
    virtual void OnAttachment();

    /** @brief Called after OnAttachment() call. */
    virtual void Initialize();

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
