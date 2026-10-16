// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareRenderingContext.h"

#include "Rendering/RObjects.h"

#include <cstddef>
#include <limits>
#include <vector>

SilverwareRenderingContext::SilverwareRenderingContext(IRenderDevice& renderDevice)
    : m_RenderDevice(renderDevice)
{
}

SilverwareRenderingContext::~SilverwareRenderingContext()
{
    for (const auto& [_, mesh] : m_Geometries)
        m_RenderDevice.DestroyMesh(mesh);

    for (const auto& [_, texture] : m_Textures)
        m_RenderDevice.DestroyTexture(texture);
}

void SilverwareRenderingContext::SetViewportSize(int width, int height)
{
    m_ViewportWidth = width;
    m_ViewportHeight = height;
}

Silverware::FGeometryHandle SilverwareRenderingContext::CreateGeometry(
    std::span<const Silverware::FRenderVertex> vertices,
    std::span<const std::uint32_t> indices)
{
    if (vertices.empty() || m_NextGeometryHandle == 0)
        return {};

    if (indices.empty() && vertices.size() % 3 != 0)
        return {};

    if (!indices.empty() && indices.size() % 3 != 0)
        return {};

    for (const std::uint32_t index : indices)
        if (index >= vertices.size())
            return {};

    RMesh mesh{};
    mesh.vertexStride = static_cast<Rint>(sizeof(float) * 8);

    mesh.attributes =
    {
        { 0, 0,                              2 },
        { 1, static_cast<Rint>(sizeof(float) * 2), 2 },
        { 2, static_cast<Rint>(sizeof(float) * 4), 4 }
    };

    mesh.vertices.reserve(vertices.size() * 8);

    for (const Silverware::FRenderVertex& vertex : vertices)
    {
        mesh.vertices.insert(mesh.vertices.end(),
        {
            vertex.x, vertex.y,
            vertex.u, vertex.v,
            vertex.r, vertex.g, vertex.b, vertex.a
        });
    }

    mesh.indices.assign(indices.begin(), indices.end());

    const RMeshHandle meshHandle = m_RenderDevice.CreateMesh(mesh);

    if (!meshHandle.IsValid())
        return {};

    const std::uint64_t id = m_NextGeometryHandle++;
    m_Geometries.emplace(id, meshHandle);

    return { id };
}

void SilverwareRenderingContext::DestroyGeometry(
    Silverware::FGeometryHandle geometry)
{
    if (!geometry.IsValid())
        return;

    const auto it = m_Geometries.find(geometry.Value);

    if (it == m_Geometries.end())
        return;

    m_RenderDevice.DestroyMesh(it->second);
    m_Geometries.erase(it);
}

Silverware::FTextureHandle SilverwareRenderingContext::CreateTexture(
    int width,
    int height,
    std::span<const std::byte> rgbaPixels)
{
    if (width <= 0 || height <= 0 || m_NextTextureHandle == 0)
        return {};

    const std::size_t w = static_cast<std::size_t>(width);
    const std::size_t h = static_cast<std::size_t>(height);
    constexpr std::size_t BytesPerPixel = 4;

    if (w > std::numeric_limits<std::size_t>::max() / BytesPerPixel / h)
        return {};

    if (rgbaPixels.size() != w * h * BytesPerPixel)
        return {};

    RTexture textureData{};
    textureData.type = ETexType::Tex2D;
    textureData.format = ETexFormat::RGBA8;
    textureData.width = width;
    textureData.height = height;
    textureData.channels = 4;
    textureData.data = rgbaPixels.data();

    textureData.bGenerateMipmaps = false;
    textureData.bUseAnisotropy = false;
    textureData.bSRGB = false;

    textureData.wrapS = ETexWrap::ClampEdge;
    textureData.wrapT = ETexWrap::ClampEdge;
    textureData.minFilter = ETexFilter::Linear;
    textureData.magFilter = ETexFilter::Linear;

    const RTextureHandle textureHandle =
        m_RenderDevice.CreateTexture(textureData);

    if (!textureHandle.IsValid())
        return {};

    const std::uint64_t id = m_NextTextureHandle++;
    m_Textures.emplace(id, textureHandle);

    return { id };
}

void SilverwareRenderingContext::DestroyTexture(
    Silverware::FTextureHandle texture)
{
    if (!texture.IsValid())
        return;

    const auto it = m_Textures.find(texture.Value);

    if (it == m_Textures.end())
        return;

    m_RenderDevice.DestroyTexture(it->second);
    m_Textures.erase(it);
}

void SilverwareRenderingContext::RenderGeometry(
    Silverware::FGeometryHandle geometry,
    Silverware::FTextureHandle texture,
    float translationX,
    float translationY)
{
    if (!geometry.IsValid() ||
        m_ViewportWidth <= 0 ||
        m_ViewportHeight <= 0)
        return;

    const auto geometryIt = m_Geometries.find(geometry.Value);

    if (geometryIt == m_Geometries.end())
        return;

    RTextureHandle textureHandle{};

    if (texture.IsValid())
    {
        const auto textureIt = m_Textures.find(texture.Value);

        if (textureIt == m_Textures.end())
            return;

        textureHandle = textureIt->second;
    }

    m_RenderDevice.SubmitScreenSpaceGeometry(
        geometryIt->second,
        textureHandle,
        translationX,
        translationY,
        m_ViewportWidth,
        m_ViewportHeight);
}

void SilverwareRenderingContext::EnableScissorRegion(bool bEnable)
{
    m_RenderDevice.EnableScissorRegion(bEnable);
}

void SilverwareRenderingContext::SetScissorRegion(int x, int y, int width, int height)
{
    m_RenderDevice.SetScissorRegion(x, y, width, height);
}