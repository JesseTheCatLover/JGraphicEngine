//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "ICpuResource.h"
#include "Core/JCoreObject.h"

class JCpuResource : public JCoreObject, public ICpuResource
{
    DECLARE_JOBJECT(JCpuResource, JCoreObject);

public:
    JCpuResource() = default;
    ~JCpuResource() override = default;
};