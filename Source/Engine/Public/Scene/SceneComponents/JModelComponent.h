// Copyright 2025 JesseTheCatLover

#pragma once
#include <memory>
#include <string>

#include "Scene/SceneComponents/JRenderableComponent.h"

class JModelResource;

/**
 * @class JModelComponent
 * @brief Temporary “heavy” component to render a full model.
 *
 * Future work splits this into Model + Mesh + Material components.
 */
class JModelComponent : public JRenderableComponent
{
    DECLARE_JOBJECT(JModelComponent, JRenderableComponent)

public:
    JModelComponent() = default;
    ~JModelComponent() override = default;

    /** Set by project-relative key; ResourceManager resolves & loads. */
    void SetModel(const std::string& modelKey);

    /** Strong read access (if needed). Prefer using EmitToRoute to draw. */
    std::shared_ptr<JModelResource> GetModel() const { return m_Model.lock(); }

    // JRenderableComponent
    void EmitToRoute(class RRenderRoute& route) const override;

protected:
    void SerializeProperties(class JsonWriter& writer) const override;
    void DeserializeProperties(const class JsonReader& reader) override;

private:
    std::string                        m_ModelKey;
    std::weak_ptr<JModelResource>      m_Model;
};
