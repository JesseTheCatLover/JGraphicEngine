//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include "IInspectProvider.h"
#include "Core/Math/FTransform.h"
#include "Core/Reflection/RETypeRegistry.h"

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
private:
    EditorHost& m_Host;

    std::unordered_map<uint64_t, FTransform> m_TransformEditBegin;
    std::unordered_map<uint64_t, REVariant> m_PropEditBegin;

public:
    explicit ActorInspectorProvider(EditorHost& host);
    ~ActorInspectorProvider();

    [[nodiscard]] uint32_t GetProviderID() const override { return 1; }
    [[nodiscard]] bool CanHandle(const FInspectorSelection& sel) const override;
    void BuildDocument(const FInspectorSelection& sel, FInspectorDocument& outDoc) override;
    void ApplyEdit(const FInspectorEditCommand& cmd) override;
};