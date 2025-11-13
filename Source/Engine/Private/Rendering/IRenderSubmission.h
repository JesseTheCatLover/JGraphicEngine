//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

struct RDrawCommand;
struct RLightData;

class IRenderSubmission
{
public:
    virtual ~IRenderSubmission() = default;

    virtual void SubmitDrawCommand(const RDrawCommand& drawCommand) = 0;
    virtual void SubmitLightData(const RLightData& lightData) = 0;
};