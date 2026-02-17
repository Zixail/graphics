#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main()
{
    float gray = vColor.r * 0.2126 + vColor.g * 0.7152 + vColor.b * 0.0722;
    FragColor = vec4(gray, 0.0, 0.0, 1.0);
}