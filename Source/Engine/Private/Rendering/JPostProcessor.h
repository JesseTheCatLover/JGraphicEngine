//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "JShader.h"

/**
 * @class JPostProcessor
 * @brief Runs a single post-processing effect (shader) on a fullscreen quad.
 *
 * Each post-processor draws a fullscreen quad with its shader, taking a
 * texture input (usually the output of a render target or a previous pass).
 */
class JPostProcessor {
public:
    /**
     * @brief Construct a post-processor with a given shader program.
     * @param vertexPath Path to vertex shader source.
     * @param fragmentPath Path to fragment shader source.
     */
    JPostProcessor(const std::string& vertexPath = "PostProcess", const std::string& fragmentPath
        = "PostProcessNoEffect");

    /**
     * @brief Apply the post-processing effect to the given texture.
     * @param inputTexture The OpenGL texture ID to process.
     * @param screenWidth Current screen width in pixels.
     * @param screenHeight Current screen height in pixels.
     *
     * This binds the input texture, runs the shader, and renders a fullscreen quad.
     */
    void Apply(unsigned int inputTexture, int screenWidth, int screenHeight);

    /** @return Reference to the shader for setting uniforms. */
    JShader& GetShader();

private:
    class JScreenQuad {
    public:
        JScreenQuad();
        ~JScreenQuad();
        void Draw() const;

    private:
        unsigned int VAO = 0, VBO = 0;
    };

    JScreenQuad ScreenQuad;
    JShader PostShader;
};
