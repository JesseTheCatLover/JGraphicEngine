//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <vector>
#include <memory>

class JFramebufferTarget;
class JPostProcessor;

/**
 * @class PostProcessManager
 * @brief Manages a chain of post-processing effects using internal ping-pong targets.
 *
 * PostProcessManager allows you to register multiple post-process effects (shaders)
 * and applies them sequentially to a rendered scene. It handles all framebuffer
 * management internally using a ping-pong technique: two offscreen render targets
 * alternate as the destination of each pass, and the output of one pass becomes
 * the input of the next.
 *
 * Example usage:
 * @code
 * PostProcessManager ppm(screenWidth, screenHeight);
 * ppm.AddProcessor(std::make_unique<JPostProcessor>("post", "Gloom"));
 * ppm.AddProcessor(std::make_unique<JPostProcessor>("post", "ColorGrading"));

* // Render scene
 * renderer.BeginScene();
 * DrawSceneObjects();
 * renderer.EndScene();
 *
 * // Apply post-processing
 * ppm.ApplyChain(renderer.GetSceneTargetTexture(), screenWidth, screenHeight);
 * @endcode
 *
 * Features:
 * - Automatic ping-pong framebuffer handling for chaining effects.
 * - Easy addition of new post-processing passes.
 * - Resizable internal targets to match dynamic screen resolutions.
 * - Safe fallback: if no processors are registered, it draws the input texture directly.
 */
class PostProcessManager {
public:
    /**
     * @brief Construct a new PostProcessManager.
     * @param width Initial width of internal render targets in pixels.
     * @param height Initial height of internal render targets in pixels.
     *
     * Initializes internal ping-pong render targets.
     */
    PostProcessManager(int width, int height);

    /**
     * @brief Add a post-processing effect to the chain.
     * @param processor Unique pointer to a JPostProcessor instance.
     *
     * The processor will be applied in order during ApplyChain().
     */
    void AddProcessor(std::unique_ptr<JPostProcessor> processor);

    /**
     * @brief Apply all registered post-processing effects.
     * @param inputTexture Texture ID of the source image (usually the scene render).
     * @param screenWidth Current screen width in pixels.
     * @param screenHeight Current screen height in pixels.
     *
     * Each processor is applied in order. Internally, ping-pong render targets
     * are used to avoid overwriting input textures. After the last pass, the
     * result is drawn to the default framebuffer.
     */
    void ApplyChain(unsigned int inputTexture, int screenWidth, int screenHeight);

    /**
     * @brief Resize internal ping-pong render targets.
     * @param newWidth New width in pixels.
     * @param newHeight New height in pixels.
     *
     * Call this when the screen resolution changes to ensure correct
     * post-processing behavior.
     */
    void Resize(int newWidth, int newHeight);

private:
    std::vector<std::unique_ptr<JPostProcessor>> Processors; ///< Registered post-process effects.
    int Width;  ///< Current width of internal render targets.
    int Height; ///< Current height of internal render targets.

    std::unique_ptr<JFramebufferTarget> Ping; ///< Ping-pong render target A.
    std::unique_ptr<JFramebufferTarget>  Pong; ///< Ping-pong render target B.
};