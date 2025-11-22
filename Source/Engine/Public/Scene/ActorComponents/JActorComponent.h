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

        bool m_bPendingDestroy = false;

    public:
        JActorComponent() = default;
        virtual ~JActorComponent() = default;

        /**
         * @brief Optional per-frame update for the component.
         * Override to add runtime logic.
         * @param deltaTime Time since last tick in seconds.
         */
        virtual void Tick(float deltaTime);

        /**
         * @brief Marks this component for destruction at the end of the frame.
         *
         * Components should never delete themselves directly. Instead, calling
         * DestroyComponent() will notify the owning actor that this component
         * should be removed. The actor will safely remove it from its internal
         * component lists (and detach it from the scene hierarchy if it is a
         * JSceneComponent) during its next destruction flush.
         *
         * This function is safe to call from within Tick(), gameplay events,
         * or any component logic. The component will continue to exist and
         * run until the end of the frame when destruction is processed.
         *
         * @return True if destruction was requested successfully.
         *         False if the component was already pending destroy.
         */
        virtual bool DestroyComponent();

        /**
         * @brief Returns whether this component has been marked for destruction.
         *
         * Components marked as pending destroy continue to exist until the
         * owning actor flushes them. This allows safe deferred cleanup without
         * invalidating iterators or interfering with ongoing gameplay logic.
         *
         * @return True if this component is awaiting destruction; otherwise false.
         */
        [[nodiscard]] bool IsPendingDestroy() const { return m_bPendingDestroy; }

        /** @brief Get the owning actor. */
        [[nodiscard]] JActor* GetOwnerActor() const { return m_OwnerActor; }

        /** @brief Get the name of the component */
        [[nodiscard]] std::string GetName() const { return m_Name; }

        /** @brief Set the name of the component */
        void SetName(const std::string& inName) { m_Name = inName; }

    protected:
        /** @brief Set the owning actor for this component. */
        void SetOwnerActor (JActor* inActor) { m_OwnerActor = inActor; }

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
        virtual void BeginPlay();

        /**
         * @brief Called when the component is ending its "play" in the scene.
         *
         * This is part of the component's lifecycle. Called by the owning actor
         * when the actor or scene is shutting down. Subclasses can override
         * this to implement cleanup or finalization logic.
         */
        virtual void EndPlay();

        /**
         * @brief Called as the final step before a component is destroyed.
         *
         * The engine invokes OnDestroy() exactly once when:
         *   - The component has been marked pending destroy, AND
         *   - The owning actor flushes destroyed components, AND
         *   - The component is about to be removed from the component list.
         *
         * Order of lifecycle:
         *   1. DestroyComponent() is called → Component is marked pending destroy.
         *   2. EndPlay() is called by the owning actor (if playing).
         *   3. OnDestroy() is invoked for cleanup.
         *   4. Component is detached from hierarchy (for scene components).
         *   5. Shared pointer is released and the component is deleted.
         *
         * Override this to:
         *   - Unregister from managers (rendering, physics, systems, events)
         *   - Release GPU handles, resources, or external references
         *   - Remove runtime bindings or delegates
         *
         * @note Do NOT manually detach or free memory here — the engine handles it.
         * @note Safe to call engine APIs here; component is still fully valid.
         */
        virtual void OnDestroy();

        /**
         * @brief Serialize this component into JSON.
         * Calls SerializeProperties() to allow subclasses to write their own fields.
         */
        void SerializeCustom(JsonWriter& writer) const override;

        /**
         * @brief Deserialize this component from JSON.
         * Calls DeserializeProperties() to allow subclasses to read their own fields.
         */
        void Deserialize(const JsonReader& reader) override;
    };
