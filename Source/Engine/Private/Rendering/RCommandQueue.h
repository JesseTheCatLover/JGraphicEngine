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

class RCommandQueue
{
private:
    std::vector<RDrawCommand> drawCommands;

public:
    void Clear() { drawCommands.clear();}
    void Submit(const RDrawCommand& c) { drawCommands.push_back(c); }
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

    // Helpers to read/replace the 16-bit depth inside 'packet'
    static inline uint16_t ExtractDepth(uint64_t p) { return uint16_t((p >> 40) & 0xFFFF); }

    static inline uint64_t ReplaceDepth(uint64_t p, uint16_t d)
    {
        const uint64_t clear = ~(0xFFFFULL << 40);
        return (p & clear) | (uint64_t(d) << 40);
    }

    // Map view-space Z to a 16-bit bucket (tweak sign to your view convention)
    static inline uint16_t DepthToBucket(float viewZ, float nearZ, float farZ)
    {
        float z = (viewZ - nearZ) / (farZ - nearZ);   // normalize 0..1
        z = std::clamp(z, 0.0f, 1.0f);
        return uint16_t(z * 65535.0f + 0.5f);
    }

    // Compute buckets for any queue that needs depth ordering
    static void ComputeDepthBucketsFor(RCommandQueue& q, const FMatrix4& view, float nearZ, float farZ)
    {
        auto& commands = q.GetDrawCommands();
        for (auto& c : commands)
        {
            if (ExtractDepth(c.packet) != 0) continue; // respect pre-filled depth

            const FVector3 worldP = c.transform.GetTranslation();
            const FVector3 viewP  = view.TransformPoint(worldP); // (View * Model) * [0,0,0,1]
            const float zVS = viewP.z; // If -Z is forward, use -viewP.z

            c.packet = ReplaceDepth(c.packet, DepthToBucket(zVS, nearZ, farZ));
        }
    }

    [[nodiscard]] std::vector<RDrawCommand>& GetDrawCommands() { return drawCommands; }
    [[nodiscard]] const std::vector<RDrawCommand>& GetDrawCommands() const { return drawCommands; }
};