#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uMask;

void main() {
    vec4 texColor = texture(uMask, vUV);
    FragColor = texColor;
}