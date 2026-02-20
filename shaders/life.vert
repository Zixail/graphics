#version 330 core
layout (location = 0) in vec2 aPos; //Координаты
layout (location = 1) in vec2 aUV; //Текстуры
layout (location = 2) in vec4 aColor; //Цвета для линий/фигур

uniform mat4 projection; // Проекция
uniform mat4 view;  //  Камера
uniform mat4 model; //  Модель
uniform int uMode; // 0 - текстура, 1 - цвет

out vec2 vUV;
out vec4 vColor;

void main() {
    vUV = aUV;
    vColor = aColor;

    vec4 worldPos = model * vec4(aPos, 0.0, 1.0);
    vec4 viewPos = view * worldPos;
    gl_Position = projection * viewPos;
}
