//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "RRenderQueue.h"

class RRenderRoute
{
private:
    std::vector<RLightData> lights;

public:
    RRenderQueue opaque; // front-to-back
    RRenderQueue alpha; // back-to-front
    RRenderQueue overlay; // submission order

    void Clear()
    {
        opaque.Clear(); alpha.Clear(); overlay.Clear(); lights.clear();
    }

    void Submit(const RDrawCommand& drawCommand)
    {
        const auto layer = ERenderLayer((drawCommand.packet >> 56) & 0xFF);
        switch (layer)
        {
            default:
            case ERenderLayer::Opaque: opaque.Submit(drawCommand); break;
            case ERenderLayer::Alpha: alpha.Submit(drawCommand); break;
            case ERenderLayer::Overlay: overlay.Submit(drawCommand); break;
        }
    }

    void SubmitLight(const RLightData& lightCommand) { lights.push_back(lightCommand); }

    void SortAllQueues()
    {
        opaque.Sort(ESortMode::FrontToBack);
        alpha.Sort(ESortMode::BackToFront);
        overlay.Sort(ESortMode::SubmissionOrder);
    }

    const std::vector<RLightData>& GetLights() const { return lights; }
};