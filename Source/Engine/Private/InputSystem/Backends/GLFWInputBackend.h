//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <GLFW/glfw3.h>
#include "../IInputBackend.h"

class GLFWInputBackend : public IInputBackend
{
public:
    explicit GLFWInputBackend(GLFWwindow* window);
    ~GLFWInputBackend() override = default;

    void FetchEvents(std::vector<FRawInputEvent>& outEvents) override;
    void GetMousePosition(float& outX, float& outY) override;

private:
    GLFWwindow* m_Window = nullptr;
    std::vector<FRawInputEvent> m_PendingEvents;

    double m_LastMouseX = 0.0;
    double m_LastMouseY = 0.0;

    // Gamepad previous states for generating deltas
    static constexpr int MAX_GAMEPADS = GLFW_JOYSTICK_LAST + 1;

    std::vector<float> m_PrevGamepadAxes[MAX_GAMEPADS];
    std::vector<unsigned char> m_PrevGamepadButtons[MAX_GAMEPADS];
    bool m_GamepadConnected[MAX_GAMEPADS] = {};

    // Static callbacks that forward into this instance
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void CharCallback(GLFWwindow* window, unsigned int codepoint);

    // Helper to get backend from window user pointer
    static GLFWInputBackend* GetBackend(GLFWwindow* window);

    void OnKey(int key, int scancode, int action, int mods);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);
    void OnCursorPos(double xpos, double ypos);
    void OnChar(unsigned int codepoint);

    // Gamepad polling
    void PollGamepads();
};