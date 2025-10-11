// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/JCoreObject.h"
#include "../Rendering/Legacy/JModel.h"
#include <memory>
#include <string>

/**
 * @class JModelResource
 * @brief Resource wrapper for model assets (collections of meshes).
 *
 * Wraps a JModel object, ensuring that model data is loaded, cached,
 * and managed by JResourceManager. Prevents duplicate instances of the
 * same model file from being created.
 *
 * Typically corresponds to an asset file such as .obj, .fbx, .gltf, etc.
 */
class JModelResource : public JCoreObject
{
    DECLARE_JOBJECT(JModelResource)

private:
    std::string m_Path;                  ///< Original file path to the model asset.
    std::shared_ptr<JModel> m_Model;     ///< The actual rendering model.

    void LoadModelFromFile(const std::string& inPath);

public:
    /**
     * @brief Construct a new JModelResource.
     * @param inPath File path to the model asset.
     */
    explicit JModelResource(const std::string& inPath);

    /**
     * @brief Get the wrapped JModel object.
     * @return Shared pointer to JModel.
     */
    std::shared_ptr<JModel> GetModel() const { return m_Model; }

    /**
     * @brief Serialize resource metadata (not full model data).
     * Only stores path and ID for scene references.
     */
    void Serialize(class JsonWriter& writer) const override;

    /**
     * @brief Deserialize resource metadata.
     * Reloads model if necessary.
     */
    void Deserialize(const class JsonReader& reader) override;
};
