#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D u_Input;
uniform sampler2D u_CustomID;
uniform sampler2D u_CustomDepth;
uniform sampler2D u_SceneDepth;

uniform vec2  u_TexelSize;        // (1 / width, 1 / height)
uniform float u_StencilRef01;     // selected ID (normalized 0..1)
uniform int   u_Mode;             // 0 = visible only, 1 = xray, 2 = both
uniform vec3  u_OutlineColor;
uniform float u_Thickness;        // pixels
uniform float u_DepthEpsilon;     // depth bias

// -------------------------

float idAt(vec2 uv)
{
    return texture(u_CustomID, uv).r;
}

bool isEdge(vec2 uv)
{
    float center = idAt(uv);
    if (abs(center - u_StencilRef01) > 0.001)
    return false;

    vec2 stepUV = u_TexelSize * u_Thickness;

    float l = idAt(uv + vec2(-stepUV.x, 0.0));
    float r = idAt(uv + vec2( stepUV.x, 0.0));
    float u = idAt(uv + vec2(0.0,  stepUV.y));
    float d = idAt(uv + vec2(0.0, -stepUV.y));

    return abs(l - center) > 0.001 ||
    abs(r - center) > 0.001 ||
    abs(u - center) > 0.001 ||
    abs(d - center) > 0.001;
}

bool isOccluded(vec2 uv)
{
    float sceneZ  = texture(u_SceneDepth,  uv).r;
    float customZ = texture(u_CustomDepth, uv).r;

    // NOTE:
    // This assumes BOTH depths are in the same space
    // (either both non-linear or both linearized).
    return (sceneZ + u_DepthEpsilon) < customZ;
}

// -------------------------

void main()
{
    vec4 base = texture(u_Input, vUV);

    if (!isEdge(vUV))
    {
        FragColor = base;
        return;
    }

    bool occluded = isOccluded(vUV);

    float alpha = 0.0;
    if (u_Mode == 0) alpha = occluded ? 0.0 : 1.0; // visible only
    if (u_Mode == 1) alpha = 1.0;                  // xray
    if (u_Mode == 2) alpha = occluded ? 0.6 : 1.0; // both

    vec3 color = mix(base.rgb, u_OutlineColor, alpha);
    FragColor = vec4(color, base.a);
}
