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

double lastUpdateTime = 0.0;
double updateInterval = 0.1;

GLuint maskTex;
unsigned char* textureData = NULL;
int textureWidth, textureHeight;

char freeze = 0;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mod){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, 1);
    }
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS){
        extern char freeze;
        freeze = !freeze;
    }
    if(key == GLFW_KEY_Z && action == GLFW_PRESS){
        extern double updateInterval;
        updateInterval *= (double)2;
        if (updateInterval > 1.6f) updateInterval = 1.6f;
    }
    if(key == GLFW_KEY_X && action == GLFW_PRESS){
        extern double updateInterval;
        updateInterval /= (double)2;
        if (updateInterval < 0.0125f) updateInterval = 0.0125f;
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

    //
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

    glfwSwapInterval(1);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, buffersizeCallback);
    buffersizeCallback(window, 1024, 768);
    glfwSetScrollCallback(window, scrollCallback);
    glClearColor(0.0f, 0.749f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    extern GLuint maskTex;
    extern unsigned char* textureData;
    extern int textureWidth, textureHeight;

    fieldInit(30, 30);
    loadSample();
    textureWidth = Field.width;
    textureHeight = Field.height;
    textureData = malloc(textureWidth * textureHeight * 4); 

    glGenTextures(1, &maskTex);
    glBindTexture(GL_TEXTURE_2D, maskTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);



    float gridTop =  1.0f * textureHeight / 20;
    float gridBot = -1.0f * textureHeight / 20;
    float gridRight =  1.0f * textureWidth / 20;
    float gridLeft =  -1.0f * textureWidth / 20;

    float quads[] = {
        gridLeft, gridBot,  0.0f, 1.0f,
        gridRight, gridBot,  1.0f, 1.0f,
        gridRight,  gridTop,  1.0f, 0.0f,
        gridLeft,  gridTop,  0.0f, 0.0f
    };

    unsigned int indices[] = { 
        0,  1,  2,    
        0,  2,  3    
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

    GLint uMaskLoc = glGetUniformLocation(shaderProgram, "uMask");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint uModeLoc = glGetUniformLocation(shaderProgram, "uMode");

    Transform qModel, qView;
    transformInit(&qModel);
    transformInit(&qView);

    int count = (Field.width + Field.height) + 2;
    float* lines = calloc(4 * count, sizeof(float));
    float dx = (gridRight - gridLeft) / textureWidth;
    float dy = (gridTop - gridBot) / textureHeight;
    for (int i = 0; i <= Field.width; ++i){
        lines[i * 4 + 0] = gridLeft + i * dx;
        lines[i * 4 + 1] = gridTop;
        lines[i * 4 + 2] = gridLeft + i * dx;
        lines[i * 4 + 3] = gridBot;
    }

    for (int i = 0; i <= Field.height; ++i){
        int base = Field.width + i + 1;
        lines[base * 4 + 0] = gridLeft;
        lines[base * 4 + 1] = gridBot + i * dy;
        lines[base * 4 + 2] = gridRight;
        lines[base * 4 + 3] = gridBot + i * dy;
    }

    GLuint lineVAO, lineVBO;

    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * count * sizeof(float), (void*)lines, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    extern float projection[16];
    extern char freeze;


    float identity[16] = {0};
    identity[0] = identity[5] = identity[10] = identity[15] = 1.0f;

    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();                
        glClear(GL_COLOR_BUFFER_BIT);      

        double currentTime = glfwGetTime();

        if(freeze && currentTime - lastUpdateTime >= updateInterval) {
            updateField();  
            lastUpdateTime = currentTime;
        }

        for(int i = 0; i < textureHeight; i++) {
            for(int j = 0; j < textureWidth; j++) {
                int fieldIdx = i * textureWidth + j;
                unsigned char cell = Field.current[fieldIdx];
                
                int texIdx = (i * textureWidth + j) * 4;
                if (cell == 0){
                    textureData[texIdx + 0] = 255; 
                    textureData[texIdx + 1] = 255;
                    textureData[texIdx + 2] = 255; 
                    textureData[texIdx + 3] = 0; 
                }
                else{
                    textureData[texIdx + 0] = 0;  
                    textureData[texIdx + 1] = 0;  
                    textureData[texIdx + 2] = 0;   
                    textureData[texIdx + 3] = 255; 
                }
            }
        }

        
        glBindTexture(GL_TEXTURE_2D, maskTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
        glUseProgram(shaderProgram);

        glUniform1i(uMaskLoc, 0);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, identity);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, identity);

        glUniform1i(uModeLoc, 1);
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, count * 2);

        glUniform1i(uModeLoc, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, maskTex);
        glBindVertexArray(qVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glfwSwapBuffers(window);             
    }

    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}