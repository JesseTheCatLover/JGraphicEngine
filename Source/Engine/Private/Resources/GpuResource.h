//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "IGpuResource.h"

class GpuResource : public IGpuResource
{
private:
    IRenderDevice* m_Device = nullptr;
    bool m_GpuCacheCreated = false;

public:
    GpuResource() = default;
    ~GpuResource() override { /* NOTE: Manager should call DestroyGpuResources() before destructor */ }

    // IGpuResource
    void SetRenderDevice(IRenderDevice* device) override { m_Device = device; }
    [[nodiscard]] bool IsGpuCacheCreated() const override { return m_GpuCacheCreated; }

    void CreateGpuResources(IRenderDevice* device) final
    {
        if (!m_GpuCacheCreated) {
            if (device) m_Device = device;
            OnCreateGpuResources(); // Implemented by derived
            m_GpuCacheCreated = true;
        }
    }
    void DestroyGpuResources(IRenderDevice* device) final
    {
        if (m_GpuCacheCreated) {
            if (device) m_Device = device;
            OnDestroyGpuResources(); // Implemented by derived
            m_GpuCacheCreated = false;
        }
    }

protected:
    // Derived classes implement the actual work, using m_Device.
    virtual void OnCreateGpuResources() = 0;
    virtual void OnDestroyGpuResources() = 0;

    [[nodiscard]] IRenderDevice* GetDevice() const { return m_Device; }
};
