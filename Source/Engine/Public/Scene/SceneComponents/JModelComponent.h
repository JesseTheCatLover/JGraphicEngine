// Copyright 2025 JesseTheCatLover

#pragma once
#include <iostream>
#include <string>

#include "Core/Memory/SmartPointers.h"
#include "Scene/SceneComponents/JRenderableComponent.h"
#include "Rendering/IRenderSubmission.h"
#include "JModelComponent.generated.h"

class ModelResource;

JCLASS()
class JModelComponent : public JRenderableComponent
{
    GENERATED_BODY()

public:
    JModelComponent() = default;
    ~JModelComponent() = default;

    /// Set by project-relative key; ResourceManager resolves & loads.
    void SetModel(const std::string& assetID);

    /// Optional: allow overriding the shader used for this model.
    void SetShader(RShaderHandle shader) { m_Shader = shader; }

    TSharedPtr<ModelResource> GetModel() const { return m_Model; }

    // JRenderableComponent
    void GatherProxies(IRenderSubmission& submission, const FRenderContext& ctx) const override;

private:
    JPROPERTY()
    std::string m_ModelKey;
    TSharedPtr<ModelResource> m_Model;
    RShaderHandle m_Shader{}; // shader used for all submeshes (for now)

protected:
    void AllocateGpuResources() override;
};
