#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float iTime = 1;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
    vec3 color = texture(screenTexture, TexCoords).rgb;
    float n = rand(TexCoords * iTime);
    color += n * 0.1;
    FragColor = vec4(color, 1.0);
}
