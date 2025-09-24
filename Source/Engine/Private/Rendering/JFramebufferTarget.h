//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <glad/gl.h>

/**
 * @class JFramebufferTarget
 * @brief Encapsulates an offscreen framebuffer with color and depth-stencil attachments.
 *
 * JFramebufferTarget provides an abstraction for offscreen rendering. It supports:
 * - Single-sample and multi-sample framebuffers (MSAA).
 * - Automatic resizing of attachments.
 * - Resolving multi-sample buffers to single-sample targets for post-processing.
 *
 * Typical usage:
 * 1. Render your scene into a JRenderTarget.
 * 2. If MSAA, resolve into a single-sample target.
 * 3. Feed the color texture into post-processing passes or a screen quad.
 * 4. Draw the final result to the default framebuffer.
 *
 * This class is ideal for use in a ping-pong post-processing chain, where
 * multiple passes swap between two targets for sequential effects.
 */
class JFramebufferTarget {
public:
    /**
     * @brief Construct a new render target.
     * @param width Width of the framebuffer in pixels.
     * @param height Height of the framebuffer in pixels.
     * @param samples Number of samples per pixel.
     *        - 1 = standard single-sample target.
     *        - >1 = multi-sampled framebuffer (MSAA).
     *
     * By default, a standard single-sample framebuffer is created.
     */
    JFramebufferTarget(int width, int height, int samples = 1);

    /**
     * @brief Destroy the render target and release all resources.
     *
     * Deletes the framebuffer, color texture, and depth-stencil renderbuffer.
     */
    ~JFramebufferTarget();

    /**
     * @brief Bind this render target for rendering.
     *
     * Automatically sets the viewport to the framebuffer size.
     */
    void Bind() const;

    /**
     * @brief Unbind this render target and restore the default framebuffer.
     * @param screenWidth Width of the default framebuffer (usually window width).
     * @param screenHeight Height of the default framebuffer (usually window height).
     *
     * Automatically updates the viewport to the given screen dimensions.
     */
    void Unbind(int screenWidth, int screenHeight) const;

    /**
     * @brief Resize the framebuffer and attachments.
     * @param newWidth New width in pixels.
     * @param newHeight New height in pixels.
     *
     * Deletes and recreates all attachments. Useful for window resizing
     * or changing the resolution of ping-pong targets.
     */
    void Resize(int newWidth, int newHeight);

    /**
     * @brief Resolve the contents of this framebuffer into another.
     * @param target Destination render target.
     *
     * Typically used to resolve a multi-sample framebuffer into a single-sample target.
     * This is required for post-processing, since shaders cannot read from multi-sampled textures.
     *
     * Example usage:
     * @code
     * MSAAFBO.ResolveTo(IntermediateTarget);
     * PostProcessor.Apply(IntermediateTarget.GetTexture(), screenWidth, screenHeight);
     * @endcode
     */
    void ResolveTo(JFramebufferTarget& target) const;

    /// Get the color attachment texture ID.
    inline GLuint GetTexture() const { return ColorTex; }

    /// Get the width of the framebuffer.
    inline int GetWidth() const { return Width; }

    /// Get the height of the framebuffer.
    inline int GetHeight() const { return Height; }

    /// Get the number of samples per pixel.
    inline int GetSamples() const { return Samples; }

private:
    GLuint FBO = 0;      ///< OpenGL framebuffer object
    GLuint ColorTex = 0; ///< Color attachment texture
    GLuint RBO = 0;      ///< Depth-stencil renderbuffer
    int Width;           ///< Framebuffer width in pixels
    int Height;          ///< Framebuffer height in pixels
    int Samples;         ///< Sample count (1 = single-sample)

    /// Allocate and attach framebuffer attachments
    void CreateBuffers();

    /// Delete all attachments
    void Destroy();
};
