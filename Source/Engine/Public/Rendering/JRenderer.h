//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <memory>

class JFramebufferTarget;

/**
 * @class JRenderer
 * @brief High-level scene renderer for offscreen rendering.
 *
 * JRenderer is responsible for rendering the 3D scene into an offscreen framebuffer.
 * It handles multisampling, depth testing, and viewport setup.
 *
 * Post-processing effects should be applied separately using a PostProcessManager
 * or similar system by using the texture provided via GetSceneTargetTexture().
 *
 * Usage pattern:
 * 1. Call BeginScene() before drawing your scene.
 * 2. Render all scene objects.
 * 3. Call EndScene() to finalize the framebuffer and resolve multisampling.
 * 4. Retrieve the final scene texture via GetSceneTargetTexture() for post-processing or presentation.
 */
class JRenderer {
public:
    /**
     * @brief Construct a new scene renderer.
     *
     * @param screenWidth Width of the target framebuffer (in pixels).
     * @param screenHeight Height of the target framebuffer (in pixels).
     * @param samples Number of samples per pixel for multisampling (default: 4).
     *        Use 1 for no multisampling.
     */
    JRenderer(int screenWidth, int screenHeight, int samples = 4);

    /**
     * @brief Begin rendering a new frame/scene.
     *
     * This binds the internal scene framebuffer, clears its color, depth, and stencil buffers,
     * and sets the viewport to match the framebuffer size.
     * Call this before drawing any scene objects.
     */
    void BeginScene();

    /**
     * @brief End scene rendering.
     *
     * Unbinds the scene framebuffer and resolves the multi-sampled scene
     * into a single-sample texture if multisampling is enabled.
     * Call this after all scene objects have been drawn.
     */
    void EndScene();

    /**
     * @brief Resize the scene framebuffer and resolved target.
     *
     * Must be called whenever the screen or window size changes to maintain correct resolution.
     *
     * @param newWidth New framebuffer width in pixels.
     * @param newHeight New framebuffer height in pixels.
     */
    void Resize(int newWidth, int newHeight);

    /**
     * @brief Get the final scene texture ID.
     *
     * This texture can be used for post-processing effects, UI overlays, or direct
     * rendering to the screen.
     *
     * @return OpenGL texture ID of the resolved scene. If multisampling is enabled,
     *         the resolved single-sample texture is returned. Otherwise, the scene texture itself.
     */
    unsigned int GetSceneTargetTexture() const;

private:
    int ScreenWidth;  ///< Current width of the framebuffer in pixels
    int ScreenHeight; ///< Current height of the framebuffer in pixels
    int Samples;      ///< Number of samples per pixel for multisampling

    std::unique_ptr<JFramebufferTarget> SceneTarget;   ///< Main render target (may be multi-sampled)
    std::unique_ptr<JFramebufferTarget> ResolveTarget; ///< Single-sample resolved target for post-processing
};
