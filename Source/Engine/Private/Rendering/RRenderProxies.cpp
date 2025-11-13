//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "RRenderProxies.h"
#include "IRenderSubmission.h"

void RMeshProxy::SubmitProxy(IRenderSubmission& submission,
const FRenderContext &ctx) const
{
    RDrawCommand cmd{};
    cmd.state.mesh     = mesh;
    cmd.state.shader   = shader;
    cmd.state.material = material;
    cmd.transform      = transform;

    cmd.packet = RCommandQueue::MakeSortKey(
        ctx.layer,
        shader.id,
        material.id,
        ctx.depthBucket
    );

    submission.SubmitDrawCommand(cmd);
}

void RLightProxy::SubmitProxy(IRenderSubmission& submission,
                              const FRenderContext &ctx) const
{
    (void)ctx; // not needed for now, but kept for future (e.g., per-view stuff)

    RLightData light{};
    light.position  = position;
    light.color     = color;
    light.intensity = intensity;

    submission.SubmitLightData(light);
}
