//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/JCoreObject.h"
#include "IGpuResource.h"

class JGpuResource : public JCoreObject, public IGpuResource
{
    DECLARE_JOBJECT(JGpuResource, JCoreObject);

private:
    IRenderDevice* m_Device = nullptr;
    bool m_GpuCacheCreated = false;

public:
    JGpuResource() = default;
    ~JGpuResource() override { /* NOTE: Manager should call DestroyGpuResources() before destructor */ }

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
