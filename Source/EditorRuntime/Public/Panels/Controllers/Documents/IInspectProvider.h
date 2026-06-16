//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <cstdint>

struct FInspectorDocument;
struct FInspectorEditCommand;

struct FInspectorSelection
{
    uint64_t runtimeID = 0;
};

class IInspectorProvider
{
public:
    virtual ~IInspectorProvider() = default;

    [[nodiscard]] virtual uint32_t GetProviderID() const = 0;
    [[nodiscard]] virtual bool CanHandle(const FInspectorSelection& sel) const = 0;

    virtual void BuildDocument(const FInspectorSelection& sel, FInspectorDocument& outDoc) = 0;
    virtual void ApplyEdit(const FInspectorEditCommand& cmd) = 0;
};