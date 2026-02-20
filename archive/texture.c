#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "transform.h"
#include "utils.h"
#include "life.h"

float projection[16];
float zoom = 1.0f;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mod){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, 1);
    }
}

void buffersizeCallback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
    extern float zoom;

    float aspect = (float)width / (float)height;

    float bottom = -1.0f * zoom;
    float top = 1.0f * zoom;

    float left = -aspect * zoom;
    float right = aspect * zoom;

    float near = -1.0f;
    float far = 1.0f;

    extern float projection[16];
    makeOrtho(left, right, bottom, top, near, far, projection);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    extern float zoom;
    float zoomSpeed = 0.1f;
    zoom -= (float)yoffset *zoomSpeed;
    if (zoom < 0.1f) zoom = 0.1f;
    if (zoom > 10.0f) zoom = 10.0f;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    buffersizeCallback(window, width, height);
}

int main(void){

    if(!glfwInit()){
        printf("glfw error");
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Test", NULL, NULL);
    if(!window){
        printf("window error");
        glfwTerminate();
        return -2;
    }

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("glad error");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -3;
    }

    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, buffersizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    int width = 10, height = 10;
    unsigned char testGrid[10*10*3];  // RGB = 3 байта на пиксель!

    for(int i = 0; i < width; i++) {
        for(int j = 0; j < height; j++) {
            int idx = (j*width + i) * 3;
            if ((i+j) % 2) {
                testGrid[idx + 0] = 255;  // R
                testGrid[idx + 1] = 255;  // G  
                testGrid[idx + 2] = 255;  // B
            } else {
                testGrid[idx + 0] = 0;    // чёрный
                testGrid[idx + 1] = 0;
                testGrid[idx + 2] = 0;
            }
        }
    }

    GLuint maskTex;
    glGenTextures(1, &maskTex);
    glBindTexture(GL_TEXTURE_2D, maskTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, 
                GL_RGB, GL_UNSIGNED_BYTE, testGrid);  // ← RGB8 вместо R8!

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);



    float quads[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

    unsigned int indices[] = { 
        0,  1,  2, 
        2,  3,  0 
    };

    GLuint qVAO, qVBO, qEBO;

    glGenVertexArrays(1, &qVAO);
    glGenBuffers(1, &qVBO);
    glGenBuffers(1, &qEBO);

    glBindVertexArray(qVAO);

    glBindBuffer(GL_ARRAY_BUFFER, qVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quads), quads, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, qEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    const char* vertexShaderSource = loadFile("shaders/life.vert");
    const char* fragmentShaderSource = loadFile("shaders/life.frag");

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint uWidthLoc = glGetUniformLocation(shaderProgram, "uWidth");
    GLint uHeightLoc = glGetUniformLocation(shaderProgram, "uHeight");
    GLint uMaskLoc = glGetUniformLocation(shaderProgram, "uMask");
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");

    extern float projection[16];


    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();                    // ← Обработка событий
        glClear(GL_COLOR_BUFFER_BIT);        // ← Очистка экрана
        
        glUseProgram(shaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, maskTex);
        glUniform1i(uMaskLoc, 0);           // ← uMaskLoc один раз!
        
        glBindVertexArray(qVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glfwSwapBuffers(window);             // ← Подмена буферов
    }

    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}