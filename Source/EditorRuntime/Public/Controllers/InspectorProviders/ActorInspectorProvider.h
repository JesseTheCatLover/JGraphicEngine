//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <vector>
#include "IInspectProvider.h"

class JCoreObject;

struct FComponentNode
{
    JCoreObject* obj = nullptr;
    std::string uuid;
    std::string parentUUID; // empty for root
    bool bIsScene = false;
};

class EditorHost;

class ActorInspectorProvider : public IInspectorProvider
{
public:
    explicit ActorInspectorProvider(EditorHost& host);
    ~ActorInspectorProvider();

    [[nodiscard]] uint32_t GetProviderID() const override { return 1; }
    [[nodiscard]] bool CanHandle(const FInspectorSelection& sel) const override;
    void BuildDocument(const FInspectorSelection& sel, FInspectorDocument& outDoc) override;
    void ApplyEdit(const FInspectorEditCommand& cmd) override;

private:
    EditorHost& m_Host;
};