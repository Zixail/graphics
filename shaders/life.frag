#version 330 core
in vec2 vUV;
in vec4 vColor;

out vec4 FragColor;

uniform sampler2D uMask;
uniform int uMode;

void main() {
    if (uMode == 0){
        FragColor = texture(uMask, vUV);
    } 
    else {
        FragColor = vColor;
    }
}
