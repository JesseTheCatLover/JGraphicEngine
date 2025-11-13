//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "JRenderableComponent.h"
#include "Core/JCoreObject.h"

class IRenderSubmission;

class JStaticMeshComponent : public JRenderableComponent
{
    DECLARE_JOBJECT(JStaticMeshComponent, JRenderableComponent);

private:
    RMeshHandle m_Mesh{};
    RShaderHandle m_Shader{};
    RMaterialHandle m_Material{};

public:
    void SetMesh(RMeshHandle m) { m_Mesh = m; }
    void SetShader(RShaderHandle s) { m_Shader = s; }
    void SetMaterial(RMaterialHandle mat) { m_Material = mat; }

    void GatherProxies(IRenderSubmission& submission, const FRenderContext& ctx) const override
    {
        if (!IsVisible() || !m_Mesh.IsValid() || !m_Shader.IsValid())
            return;

        RDrawCommand cmd{};
        cmd.state.mesh = m_Mesh;
        cmd.state.shader = m_Shader;
        cmd.state.material = m_Material;
        cmd.transform = GetWorldTransform().ToMatrix();

        const uint16_t depthBucket = 0;
        cmd.packet = RCommandQueue::MakeSortKey(
            GetRenderLayer(), cmd.state.shader.id, cmd.state.material.id, depthBucket);

        submission.SubmitDrawCommand(cmd);
    }
    // TODO: Serialization
};
