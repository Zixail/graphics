#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 color;

out vec3 vColor;

void main()
{
    vec4 worldPos = model * vec4(aPos, 0.0, 1.0);
    gl_Position = projection * view * worldPos;
    vColor = color;
}
