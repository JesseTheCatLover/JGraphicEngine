// Copyright 2025 JesseTheCatLover

#pragma once
#include <iostream>
#include <string>

#include "Core/Memory/SmartPointers.h"
#include "Scene/SceneComponents/JRenderableComponent.h"
#include "Rendering/IRenderSubmission.h"

class JModelResource;

class JModelComponent : public JRenderableComponent
{
    DECLARE_JOBJECT(JModelComponent, JRenderableComponent)

public:
    JModelComponent() = default;
    ~JModelComponent() = default;

    /// Set by project-relative key; ResourceManager resolves & loads.
    void SetModel(const std::string& assetID);

    /// Optional: allow overriding the shader used for this model.
    void SetShader(RShaderHandle shader) { m_Shader = shader; }

    TSharedPtr<JModelResource> GetModel() const { return m_Model; }

    // JRenderableComponent
    void GatherProxies(IRenderSubmission& submission, const FRenderContext& ctx) const override;

protected:
    void Serialize(class JsonWriter& writer) const override;
    void Deserialize(const class JsonReader& reader) override;

private:
    std::string m_ModelKey;
    TSharedPtr<JModelResource> m_Model;
    RShaderHandle m_Shader{}; // shader used for all submeshes (for now)
};
