// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JRenderer.h"

#include "IRenderBackend.h"
#include "RRenderProxies.h"

void JRenderer::BeginScene()
{
    m_Backend->BeginFrame();
}


void JRenderer::EndScene()
{
    m_Backend->EndFrame();
}

void JRenderer::Shutdown()
{
    m_Backend->Shutdown();
}

void JRenderer::SubmitProxy(RRenderProxy *proxy)
{
    if (proxy)
    {
        proxy->Submit(m_Backend);
    }
}
