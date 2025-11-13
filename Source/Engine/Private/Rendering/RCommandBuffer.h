//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "IRenderSubmission.h"
#include "RCommandQueue.h"

class RCommandBuffer : public IRenderSubmission
{
private:
    std::vector<RLightData> lights;

public:
    RCommandQueue opaque; // front-to-back
    RCommandQueue alpha; // back-to-front
    RCommandQueue overlay; // submission order

    void Clear()
    {
        opaque.Clear(); alpha.Clear(); overlay.Clear(); lights.clear();
    }

    void SortAllQueues()
    {
        opaque.Sort(ESortMode::FrontToBack);
        alpha.Sort(ESortMode::BackToFront);
        overlay.Sort(ESortMode::SubmissionOrder);
    }

    const std::vector<RLightData>& GetLights() const { return lights; }

    void SubmitDrawCommand(const RDrawCommand& drawCommand) override
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

    void SubmitLightData (const RLightData& lightCommand) override { lights.push_back(lightCommand); }
};