// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>

struct RRenderProxy;
class IRenderBackend;

class JRenderer
{
    friend class JEngine;

private:
    explicit JRenderer(IRenderBackend* backend) { m_Backend = backend; }

    IRenderBackend* m_Backend = nullptr;
    std::vector<RRenderProxy*> m_Proxies; // gathered each frame

    void BeginScene();
    void EndScene();
    void Shutdown();

public:
    ~JRenderer() = default;

    void SubmitProxy(RRenderProxy* proxy);
};
