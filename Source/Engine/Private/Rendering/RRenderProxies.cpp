//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "RRenderProxies.h"
#include "IRenderBackend.h"
#include "RRenderRoute.h"

void RMeshProxy::RecordToRoute(RRenderRoute& route, const RRenderContext &ctx)
{
    RDrawCommand cmd{};
    cmd.state.mesh = mesh;
    cmd.state.shader = shader;
    cmd.state.material = material;
    cmd.transform = transform;
    cmd.packet = RRenderQueue::MakeSortKey(ctx.layer, shader.id, material.id, ctx.depthBucket);
    route.Submit(cmd);
}

void RLightProxy::RecordToRoute(RRenderRoute& route, const RRenderContext &ctx)
{
    (void)ctx;
    route.SubmitLight(RLightData{ position, intensity, color});
}
