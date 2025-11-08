//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Scene/Components/JSceneComponent.h"
#include "Rendering/RRenderQueue.h"

class RRenderRoute;

class JRenderableComponent : public JSceneComponent
{
    DECLARE_JOBJECT(JRenderableComponent, JSceneComponent);

private:
    bool m_Visible{true};
    ERenderLayer m_RenderLayer{ERenderLayer::Opaque};

    RMeshHandle m_Mesh{};
    RShaderHandle m_Shader{};
    RMaterialHandle m_Material{};

public:
    JRenderableComponent() = default;
    ~JRenderableComponent() override = default;

    void SetVisible(bool bVisible) { m_Visible = bVisible; }
    bool IsVisible() const { return m_Visible; }

protected:
    void SetRenderLayer(ERenderLayer layer) { m_RenderLayer = layer; }
    ERenderLayer GetRenderLayer() const { return m_RenderLayer; }

    void SetMeshHandle(RMeshHandle handle) { m_Mesh = handle; }
    void SetShaderHandle(RShaderHandle handle) { m_Shader = handle; }
    void SetMaterialHandle(RMaterialHandle handle) { m_Material = handle; }

    virtual bool CanRender() const { return m_Visible && m_Mesh.IsValid() && m_Shader.IsValid(); }

    /**
     * @brief Entry point called by the scene traversal / renderer pass.
     * @note Default implementation emits exactly one draw. */
    virtual void EmitToRoute(RRenderRoute& route) const;

    void SerializeProperties(JsonWriter &writer) const override;
    void DeserializeProperties(const JsonReader &reader) override;
};
