//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "GLFWInputBackend.h"
#include <cmath>

GLFWInputBackend::GLFWInputBackend(GLFWwindow* window)
    : m_Window(window)
{
    // Store this backend instance on the window so static callbacks can find it
    glfwSetWindowUserPointer(m_Window, this);

    // Register GLFW callbacks
    glfwSetKeyCallback (m_Window, &GLFWInputBackend::KeyCallback);
    glfwSetMouseButtonCallback(m_Window, &GLFWInputBackend::MouseButtonCallback);
    glfwSetScrollCallback (m_Window, &GLFWInputBackend::ScrollCallback);
    glfwSetCursorPosCallback (m_Window, &GLFWInputBackend::CursorPosCallback);
    glfwSetCharCallback(m_Window, &GLFWInputBackend::CharCallback);

    // Initialize last mouse position
    glfwGetCursorPos(m_Window, &m_LastMouseX, &m_LastMouseY);
}

void GLFWInputBackend::FetchEvents(std::vector<FRawInputEvent>& outEvents)
{
    // glfwPollEvents() is called from the GLFWSurface.

    // Poll gamepads every frame (no callbacks in GLFW for them)
    PollGamepads();

    // Move pending events out
    outEvents = std::move(m_PendingEvents);
    m_PendingEvents.clear();
}

void GLFWInputBackend::GetMousePosition(float& outX, float& outY)
{
    if (!m_Window)
    {
        outX = outY = 0.0f;
        return;
    }
    double x, y;
    glfwGetCursorPos(m_Window, &x, &y);
    outX = static_cast<float>(x);
    outY = static_cast<float>(y);
}

// ---------------------- Static helpers ----------------------

GLFWInputBackend* GLFWInputBackend::GetBackend(GLFWwindow* window)
{
    return static_cast<GLFWInputBackend*>(glfwGetWindowUserPointer(window));
}

void GLFWInputBackend::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    GLFWInputBackend* backend = GetBackend(window);
    if (!backend)
        return;

    backend->OnKey(key, scancode, action, mods);
}

void GLFWInputBackend::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    GLFWInputBackend* backend = GetBackend(window);
    if (!backend)
        return;

    backend->OnMouseButton(button, action, mods);
}

void GLFWInputBackend::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    GLFWInputBackend* backend = GetBackend(window);
    if (!backend)
        return;

    backend->OnScroll(xoffset, yoffset);
}

void GLFWInputBackend::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    GLFWInputBackend* backend = GetBackend(window);
    if (!backend)
        return;

    backend->OnCursorPos(xpos, ypos);
}

void GLFWInputBackend::CharCallback(GLFWwindow* window, unsigned int codepoint)
{
    GLFWInputBackend* backend = GetBackend(window);
    if (!backend)
        return;

    backend->OnChar(codepoint);
}

// ---------------------- Instance handlers ----------------------

void GLFWInputBackend::OnKey(int key, int scancode, int action, int mods)
{
    // Ignore unknown keys
    if (key == GLFW_KEY_UNKNOWN)
        return;

    FRawInputEvent ev{};
    ev.deviceID  = 0; // single keyboard for now
    ev.code      = static_cast<uint32_t>(key);
    ev.timestamp = glfwGetTime();

    if (action == GLFW_PRESS)
    {
        ev.type  = ERawInputType::KeyDown;
        ev.value = 1.0f;
    }
    else if (action == GLFW_RELEASE)
    {
        ev.type  = ERawInputType::KeyUp;
        ev.value = 0.0f;
    }
    else if (action == GLFW_REPEAT)
    {
        // Option 1: treat repeat as another KeyDown
        ev.type  = ERawInputType::KeyDown;
        ev.value = 1.0f;
    }
    else
    {
        return;
    }

    m_PendingEvents.push_back(ev);
}

void GLFWInputBackend::OnMouseButton(int button, int action, int mods)
{
    FRawInputEvent ev{};
    ev.deviceID  = 0; // single mouse
    ev.code      = static_cast<uint32_t>(button);
    ev.timestamp = glfwGetTime();

    if (action == GLFW_PRESS)
    {
        ev.type  = ERawInputType::MouseButtonDown;
        ev.value = 1.0f;
    }
    else if (action == GLFW_RELEASE)
    {
        ev.type  = ERawInputType::MouseButtonUp;
        ev.value = 0.0f;
    }
    else
    {
        return;
    }

    m_PendingEvents.push_back(ev);
}

void GLFWInputBackend::OnScroll(double xoffset, double yoffset)
{
    const double time = glfwGetTime();

    // Vertical scroll (wheel up/down) -> axis 0
    if (yoffset != 0.0)
    {
        FRawInputEvent ev{};
        ev.deviceID  = 0;
        ev.type      = ERawInputType::MouseWheel;
        ev.code      = 0; // axis 0 = vertical
        ev.value     = static_cast<float>(yoffset);
        ev.timestamp = time;
        m_PendingEvents.push_back(ev);
    }

    // Horizontal scroll -> MouseWheel axis 1
    if (xoffset != 0.0)
    {
        FRawInputEvent ev{};
        ev.deviceID  = 0;
        ev.type      = ERawInputType::MouseWheel;
        ev.code      = 1; // axis 1 = horizontal
        ev.value     = static_cast<float>(xoffset);
        ev.timestamp = time;
        m_PendingEvents.push_back(ev);
    }
}

void GLFWInputBackend::OnCursorPos(double xpos, double ypos)
{
    const double time = glfwGetTime();

    double dx = xpos - m_LastMouseX;
    double dy = ypos - m_LastMouseY;

    m_LastMouseX = xpos;
    m_LastMouseY = ypos;

    // If there's no movement, don't spam events
    if (dx != 0.0)
    {
        FRawInputEvent ev{};
        ev.deviceID  = 0;
        ev.type      = ERawInputType::MouseMove;
        ev.code      = 2; // axis 2 = deltaX (matches ProcessEvents)
        ev.value     = static_cast<float>(dx);
        ev.timestamp = time;
        m_PendingEvents.push_back(ev);
    }

    if (dy != 0.0)
    {
        FRawInputEvent ev{};
        ev.deviceID  = 0;
        ev.type      = ERawInputType::MouseMove;
        ev.code      = 3; // axis 3 = deltaY (matches ProcessEvents)
        ev.value     = static_cast<float>(dy);
        ev.timestamp = time;
        m_PendingEvents.push_back(ev);
    }
}

void GLFWInputBackend::OnChar(unsigned int codepoint)
{
    FRawInputEvent ev{};
    ev.deviceID  = 0;
    ev.type      = ERawInputType::TextInput;
    ev.timestamp = glfwGetTime();

    ev.code  = static_cast<uint32_t>(codepoint);
    ev.value = 0.0f;

    m_PendingEvents.push_back(ev);
}

// ---------------------- Gamepad polling ----------------------

void GLFWInputBackend::PollGamepads()
{
    const double time = glfwGetTime();
    constexpr float AxisEpsilon = 0.001f;

    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid)
    {
        const bool bIsNowConnect = (glfwJoystickIsGamepad(jid) == GLFW_TRUE);
        bool& bWasConnected = m_GamepadConnected[jid];

        // ---------------- DISCONNECTED THIS FRAME ----------------
        if (!bIsNowConnect && bWasConnected)
        {
            // 1) Mark as disconnected
            bWasConnected = false;

            // 2) Clear previous state so we don't use stale data
            m_PrevGamepadButtons[jid].clear();
            m_PrevGamepadAxes[jid].clear();

            // 3) TODO: future, explicit events, extend ERawInputType with
            // GamepadConnected / GamepadDisconnected and emit one here.
            //
            // FRawInputEvent ev{};
            // ev.deviceID  = jid;
            // ev.type      = ERawInputType::GamepadDisconnected;
            // ev.code      = 0;
            // ev.value     = 0.0f;
            // ev.timestamp = time;
            // m_PendingEvents.push_back(ev);

            continue; // nothing more to do for this jid
        }

        // ---------------- NOT A GAMEPAD / NEVER WAS ----------------
        if (!bIsNowConnect && !bWasConnected)
        {
            // still not a gamepad, skip
            continue;
        }

        // ---------------- NEWLY CONNECTED THIS FRAME ----------------
        if (bIsNowConnect && !bWasConnected)
        {
            bWasConnected = true;

            // Optionally emit a "connected" event if you add it to ERawInputType.
            //
            // FRawInputEvent ev{};
            // ev.deviceID  = jid;
            // ev.type      = ERawInputType::GamepadConnected;
            // ev.code      = 0;
            // ev.value     = 0.0f;
            // ev.timestamp = time;
            // m_PendingEvents.push_back(ev);
        }

        // If we reach here, we know: bIsNowConnect == true and bWasConnected == true (currently connected)

        GLFWgamepadstate state{};
        if (glfwGetGamepadState(jid, &state) != GLFW_TRUE)
            continue;

        const int deviceID = jid; // This will be used as FRawInputEvent::deviceID

        // --- Buttons ---
        auto& prevButtons = m_PrevGamepadButtons[jid];
        if (prevButtons.size() != GLFW_GAMEPAD_BUTTON_LAST + 1)
            prevButtons.assign(GLFW_GAMEPAD_BUTTON_LAST + 1, GLFW_RELEASE);

        for (int b = 0; b <= GLFW_GAMEPAD_BUTTON_LAST; ++b)
        {
            unsigned char prev = prevButtons[b];
            unsigned char curr = state.buttons[b];

            if (curr == prev)
                continue;

            FRawInputEvent ev{};
            ev.deviceID  = deviceID;
            ev.code      = static_cast<uint32_t>(b); // button index
            ev.timestamp = time;

            if (curr == GLFW_PRESS)
            {
                ev.type  = ERawInputType::GamepadButtonDown;
                ev.value = 1.0f;
            }
            else // GLFW_RELEASE
            {
                ev.type  = ERawInputType::GamepadButtonUp;
                ev.value = 0.0f;
            }

            m_PendingEvents.push_back(ev);
            prevButtons[b] = curr;
        }

        // --- Axes ---
        auto& prevAxes = m_PrevGamepadAxes[jid];
        if (prevAxes.size() != GLFW_GAMEPAD_AXIS_LAST + 1)
            prevAxes.assign(GLFW_GAMEPAD_AXIS_LAST + 1, 0.0f);

        for (int a = 0; a <= GLFW_GAMEPAD_AXIS_LAST; ++a)
        {
            float prev = prevAxes[a];
            float curr = state.axes[a];

            if (std::abs(curr - prev) < AxisEpsilon)
                continue;

            FRawInputEvent ev{};
            ev.deviceID  = deviceID;
            ev.type      = ERawInputType::GamepadAxis;
            ev.code      = static_cast<uint32_t>(a);   // axis index
            ev.value     = curr;                       // -1..1
            ev.timestamp = time;

            m_PendingEvents.push_back(ev);
            prevAxes[a] = curr;
        }
    }
}