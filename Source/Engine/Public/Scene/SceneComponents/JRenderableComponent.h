// Copyright 2025 JesseTheCatLover

#pragma once

#include "Scene/SceneComponents/JSceneComponent.h"
#include "Rendering/IRenderSubmission.h"
#include "Rendering/RRenderProxies.h"
#include "Rendering/RCommandQueue.h"

class IRendererSubmission;

class JRenderableComponent : public JSceneComponent
{
    DECLARE_JOBJECT(JRenderableComponent, JSceneComponent)

private:
    bool m_Visible{true};
    ERenderLayer m_RenderLayer{ERenderLayer::Opaque};

public:
    void SetVisible(bool v) { m_Visible = v; }
    bool IsVisible() const { return m_Visible; }

    void SetRenderLayer(ERenderLayer layer) { m_RenderLayer = layer; }
    ERenderLayer GetRenderLayer() const { return m_RenderLayer; }

    virtual void GatherProxies(IRenderSubmission& submission, const FRenderContext& ctx) const = 0;
};
