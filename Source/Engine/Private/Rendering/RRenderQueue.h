//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include "RHandles.h"
#include "Core/Math/FMatrix4.h"
#include "Core/Math/FVector3.h"

enum class ERenderLayer : uint8_t { Opaque = 0, Alpha = 1, Overlay = 2};
enum class ESortMode { FrontToBack, BackToFront, SubmissionOrder };

struct FGPUStateCache
{
    RShaderHandle shader{};
    RMeshHandle mesh{};
    RMaterialHandle material{};
};

struct RDrawCommand
{
    uint64_t packet{0};   // composed key (layer, shader, material, depth bucket, etc.)
    FGPUStateCache state{};
    FMatrix4 transform{FMatrix4::Identity()};
};

struct RLightData
{
    // Simple straightforward lighting for now
    FVector3 position{};
    float intensity{1.f};
    FVector3 color{1, 1, 1};
};

class RRenderQueue
{
private:
    std::vector<RDrawCommand> drawCommands;
    std::vector<RLightData> lights;

public:
    void Clear() { drawCommands.clear(); lights.clear(); }
    void Submit(const RDrawCommand& c) { drawCommands.push_back(c); }
    void SubmitLight(const RLightData& L) { lights.push_back(L); }
    void Sort(ESortMode mode = ESortMode::FrontToBack)
    {
        if (mode == ESortMode::SubmissionOrder) return; // keep submission order (e.g., overlay)

        auto KeyDepth = [](uint64_t k){ return uint16_t((k >> 40) & 0xFFFF); };

        std::stable_sort(drawCommands.begin(), drawCommands.end(),
        [&](const RDrawCommand& a, const RDrawCommand& b)
        {
            if (mode == ESortMode::FrontToBack) {
                return a.packet < b.packet; // your key already packs depth ascending
            }
            else
            { // BackToFront: invert depth, preserve other ordering
                uint16_t da = KeyDepth(a.packet), db = KeyDepth(b.packet);
                if (da != db) return da > db; // reverse depth only
                return a.packet < b.packet; // then state buckets
            }
        });
    }

    static uint64_t MakeSortKey(ERenderLayer layer, uint32_t shaderId, uint32_t materialId, uint16_t depthBucket)
    {
        // [ 8 bits layer | 16 bits depth | 20 bits shader | 20 bits material ]
        return ( (uint64_t(layer) << 56) |
                 (uint64_t(depthBucket) << 40) |
                 (uint64_t(shaderId & 0xFFFFF) << 20) |
                 (uint64_t(materialId & 0xFFFFF)) );
    }

    [[nodiscard]] std::vector<RDrawCommand>& GetDrawCommands() { return drawCommands; }
    [[nodiscard]] std::vector<RLightData>& GetLights() { return lights; }
};