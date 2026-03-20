//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "CpuResource.h"
#include "IGpuResource.h"

class GpuResource : public CpuResource, public IGpuResource
{
private:
    IRenderDevice* m_Device = nullptr;
    bool m_GpuCacheCreated = false;

public:
    explicit GpuResource(AssetRegistrySubsystem* registry)
        : CpuResource(registry)
    {}
    ~GpuResource() override { /* NOTE: Manager should call DestroyGpuResources() before destructor */ }

    // IGpuResource
    void SetRenderDevice(IRenderDevice* device) override { m_Device = device; }
    [[nodiscard]] bool IsGpuCacheCreated() const override { return m_GpuCacheCreated; }

    void CreateGpuResources(IRenderDevice* device) final
    {
        if (m_GpuCacheCreated)
            return;

        if (device)
            m_Device = device;

        if (!m_Device)
            return;

        if (OnCreateGpuResources()) // Implement by derived
            m_GpuCacheCreated = true;
    }

    void DestroyGpuResources() final
    {
        if (m_GpuCacheCreated)
        {
            OnDestroyGpuResources(); // Implemented by derived
            m_GpuCacheCreated = false;
        }
    }

protected:
    // Derived classes implement the actual work, using m_Device.
    virtual bool OnCreateGpuResources() = 0;
    virtual void OnDestroyGpuResources() = 0;

    [[nodiscard]] IRenderDevice* GetDevice() const { return m_Device; }
};
