#pragma once
#include "JShader.h"

/**
 * @class JPostProcessor
 * @brief Handles framebuffer-based post-processing effects.
 *
 * This class encapsulates an offscreen framebuffer and a fullscreen quad to
 * allow applying post-processing shaders to a rendered scene. The framebuffer
 * and quad are fully private; the user interacts only through Begin(), End(),
 * and the shader accessor.
 */
class JPostProcessor {
private:
    /**
     * @class JFrameBuffer
     * @brief Internal helper for creating and managing an offscreen framebuffer.
     *
     * Contains a color texture and a renderbuffer for depth + stencil. Provides
     * Bind() and Unbind() methods to control OpenGL state.
     */
    class JFrameBuffer {
    public:
        unsigned int FBO = 0;      ///< OpenGL framebuffer object ID
        unsigned int ColorTex = 0; ///< Color texture attachment
        unsigned int RBO = 0;      ///< Depth + stencil renderbuffer
        int Width, Height;         ///< Framebuffer dimensions

        JFrameBuffer(int width, int height);
        ~JFrameBuffer();

        /** Bind framebuffer for rendering */
        void Bind() const;

        /** Unbind framebuffer and reset viewport */
        void Unbind(int screenWidth, int screenHeight) const;

        /** Resize the framebuffer and its attachments to a new width and height */
        void Resize(int newWidth, int newHeight);
    };

    /**
     * @class JScreenQuad
     * @brief Internal helper for drawing a fullscreen quad.
     *
     * Handles VAO and VBO setup and draws a two-triangle quad that spans
     * normalized device coordinates (-1 to 1) in X and Y.
     */
    class JScreenQuad {
    public:
        JScreenQuad();
        ~JScreenQuad();

        /** Draw the fullscreen quad */
        void Draw() const;

    private:
        unsigned int VAO = 0, VBO = 0; ///< Vertex array and buffer objects
    };

public:
    /**
     * @brief Construct a new post-processor.
     * @param width Width of the offscreen framebuffer in pixels.
     * @param height Height of the offscreen framebuffer in pixels.
    *  @param vertexPath Path to the vertex shader
     * @param fragmentPath Path to the fragment shader
     */
    JPostProcessor(int width, int height, const std::string& vertexPath, const std::string& fragmentPath);

    /**
     * @brief Bind the framebuffer for rendering the scene.
     *
     * All subsequent draw calls will write into this framebuffer until End() is called.
     */
    void Begin();

    /**
     * @brief Unbind the framebuffer and render it to the screen with the post-processing shader.
     * @param screenWidth Width of the screen / window in pixels.
     * @param screenHeight Height of the screen / window in pixels.
     *
     * This function clears the screen and draws a fullscreen quad with the framebuffer's
     * color texture using the configured post-processing shader.
     */
    void End(int screenWidth, int screenHeight);

    /**
    * @brief Resize the framebuffer and its attachments to new dimensions.
    * @param newWidth The new width of the framebuffer in pixels.
    * @param newHeight The new height of the framebuffer in pixels.
    *
    * This updates the color texture and the depth/stencil renderbuffer to match
    * the new size. Call this whenever the window is resized to maintain proper rendering.
    */
    void Resize(int newWidth, int newHeight);

    /**
     * @brief Access the post-processing shader to update uniforms.
     * @return Reference to the JShader object used for post-processing.
     *
     * You can use this to set kernel matrices, time variables, or other uniforms
     * before calling End().
     */
    JShader& GetShader();

    /**
     * @brief Get the framebuffer color texture.
     * @return OpenGL texture ID of the framebuffer's color attachment.
     *
     * Useful if you need the texture for further processing or saving.
     */
    unsigned int GetTexture() const;

    /**
     * @brief Get a reference to the internal framebuffer object.
     * @return Reference to the private JFrameBuffer.
     *
     * Allows advanced users to manipulate the framebuffer directly
     * for multi-pass effects or custom rendering.
     */
    JFrameBuffer& GetFramebuffer() { return FBO; }

    /**
     * @brief Get a reference to the internal screen quad.
     * @return Reference to the private JScreenQuad.
     *
     * Useful if you want to render the quad manually, or reuse it
     * for custom effects outside of End().
     */
    JScreenQuad& GetScreenQuad() { return ScreenQuad; }

private:

    // --- Members ---
    JFrameBuffer FBO;       ///< Offscreen framebuffer
    JScreenQuad ScreenQuad; ///< Fullscreen quad
    JShader PostShader;     ///< Shader used for post-processing
};
