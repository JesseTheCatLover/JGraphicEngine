#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int samples = 5;

void main() {
    vec2 center = vec2(0.5, 0.5);
    vec2 dir = TexCoords - center;
    vec3 color = vec3(0.0);
    for(int i = 0; i < samples; i++) {
        color += texture(screenTexture, TexCoords - dir * float(i) * 0.02).rgb;
    }
    color /= float(samples);
    FragColor = vec4(color, 1.0);
}
