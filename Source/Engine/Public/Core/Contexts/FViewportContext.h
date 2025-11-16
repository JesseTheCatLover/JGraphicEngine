#include "Scene/JCamera.h"

class JCameraComponent;

struct FViewportContext
{
private:
    friend class EngineState;

    bool bWireframe = false;
    EViewMode viewMode = EViewMode::Scene;

    JCameraComponent* camera;
    float xSensitivity = 0.1f;
    float ySensitivity = 0.1f;
    float MaxFOV = 45.f;
    float Zoom = 45.f;

public:
    float GetXSensitivity() const { return xSensitivity; }
    void SetXSensitivity(float sens) { xSensitivity = sens; }

    float GetYSensitivity() const { return ySensitivity; }
    void SetYSensitivity(float sens) { ySensitivity = sens; }

    float GetMaxFOV() const { return MaxFOV; }
    void SetMaxFOV(float fov) { MaxFOV = fov; }

    float GetZoom() const { return Zoom; }
    void SetZoom(float zoom) { Zoom = zoom; }
};
