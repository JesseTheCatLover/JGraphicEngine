//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "RRenderProxies.h"

#include "IRenderBackend.h"

void RMeshProxy::Submit(IRenderBackend *backend)
{
    backend->BindShader(shader);
    backend->SubmitMesh(mesh, shader, transform);
}

void RLightProxy::Submit(IRenderBackend *backend)
{
    {
        backend->SubmitLight(position, color, intensity);
    }
}
