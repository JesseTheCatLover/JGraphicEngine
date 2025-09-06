#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float intensity = 0.1;
uniform float frequency = 800.0;

void main() {
    vec3 color = texture(screenTexture, TexCoords).rgb;
    float scan = sin(TexCoords.y * frequency) * intensity;
    color *= (1.0 - scan);
    FragColor = vec4(color, 1.0);
}
