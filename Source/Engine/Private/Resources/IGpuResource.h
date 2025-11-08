//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
class IRenderDevice;

class IGpuResource {
public:
    virtual ~IGpuResource() = default;

    // Called once the resource has the render device. Should allocate GPU objects.
    virtual void CreateGpuResources(IRenderDevice* device) = 0;

    // Called before dropping the last engine reference. Should free GPU objects.
    virtual void DestroyGpuResources(IRenderDevice* device) = 0;

    // Wire the render device into the resource (before CreateGpuResources).
    virtual void SetRenderDevice(IRenderDevice* device) = 0;

    // Optional: query whether GPU cache is currently allocated (for safety/debug).
    virtual bool IsGpuCacheCreated() const = 0;
};

