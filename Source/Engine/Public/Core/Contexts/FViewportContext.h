#include "Scene/JCamera.h"

struct FViewportContext
{
private:
    friend class EngineState;

    bool bWireframe = false;
    EViewMode viewMode = EViewMode::Scene;

    JCamera camera;
    float Yaw = -90.f;
    float Pitch = 0.f;
    float Speed = 8.5f;
    float xSensitivity = 0.1f;
    float ySensitivity = 0.1f;
    float MaxFOV = 45.f;
    float Zoom = 45.f;

public:
    float GetYaw() const { return Yaw; }
    void SetYaw(float yaw) { Yaw = yaw; }

    float GetPitch() const { return Pitch; }
    void SetPitch(float pitch) { Pitch = pitch; }

    float GetSpeed() const { return Speed; }
    void SetSpeed(float speed) { Speed = speed; }

    float GetXSensitivity() const { return xSensitivity; }
    void SetXSensitivity(float sens) { xSensitivity = sens; }

    float GetYSensitivity() const { return ySensitivity; }
    void SetYSensitivity(float sens) { ySensitivity = sens; }

    float GetMaxFOV() const { return MaxFOV; }
    void SetMaxFOV(float fov) { MaxFOV = fov; }

    float GetZoom() const { return Zoom; }
    void SetZoom(float zoom) { Zoom = zoom; }
};
