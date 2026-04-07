// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "Core/Memory/SmartPointers.h"
#include "Scene/SceneComponents/JRenderableComponent.h"
#include "Rendering/RHandles.h"
#include "JStaticMeshComponent.generated.h"

class AssetRegistrySubsystem;
struct FRenderContext;
class IRenderSubmission;
class StaticMeshResource;
class MaterialResource;
class ResourceSubsystem;

JCLASS()
class JStaticMeshComponent : public JRenderableComponent
{
    GENERATED_BODY()

public:
    JStaticMeshComponent() = default;
    virtual ~JStaticMeshComponent() = default;

    /// Assign a static mesh by asset ID.
    void SetStaticMesh(const std::string& assetID);

    /// Optional: override shader for this mesh.
    void SetShader(RShaderHandle shader) { m_Shader = shader; }

    TSharedPtr<StaticMeshResource> GetStaticMesh() const { return m_StaticMesh; }

    /// Per-material-slot override: index must be < mesh material slot count.
    void SetMaterialOverride(size_t slotIndex, const std::string& materialAssetID);

    // JRenderableComponent
    void GatherProxies(IRenderSubmission& submission, const FRenderContext& ctx) const override;

protected:
    void AllocateGpuResources() override;

private:
    void ResolveMaterials() const; // lazily resolve on demand

private:
    JPROPERTY(HiddenInInspector)
    std::string m_StaticMeshAssetID;

    // Cached static mesh resource
    TSharedPtr<StaticMeshResource> m_StaticMesh;

    // Optional shader override (single shader for all submeshes)
    RShaderHandle m_Shader{};

    // Per-slot override asset IDs
    JPROPERTY()
    std::vector<std::string> m_MaterialOverrideAssetIDs;

    // Cached resolved material handles (GPU materials) per slot
    mutable std::vector<RMaterialHandle> m_ResolvedMaterials;
    mutable bool m_bMaterialsResolved = false;

    ResourceSubsystem* m_ResourceSubsystem = nullptr;
    AssetRegistrySubsystem* m_AssetRegistry = nullptr;
};