#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "transform.h"
#include "input.h"
#include "render.h"
#include "life.h"

float zoom = 1.0f;
double lastUpdateTime = 0.0;
double updateInterval = 0.1;
char freeze = 0;
char isLeftMouseButtonPressed = 0;
char isFullscreen = 0;
int windowedX, windowedY, windowedWidth, windowedHeight;

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
    if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        isFullscreen = !isFullscreen;
        
        if (isFullscreen) {
            glfwGetWindowPos(window, &windowedX, &windowedY);
            glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
            
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetWindowMonitor(window, NULL, windowedX, windowedY, windowedWidth, windowedHeight, 0);
        }
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

    extern Projection proj;
    makeOrtho(left, right, bottom, top, near, far, proj.mat);
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

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {                
            processMouseClick(window, 1);               
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {                
            processMouseClick(window, 0);               
        }                
    }
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {                
        processMouseClick(window, 1);              
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {                
        processMouseClick(window, 0);               
    }
}

void processMouseClick(GLFWwindow* window, int action){
    extern Projection proj;
    extern int textureWidth, textureHeight;
    extern struct _grid Grid;

    double xpos, ypos;                
    glfwGetCursorPos(window, &xpos, &ypos);                

    int width, height;                
    glfwGetWindowSize(window, &width, &height);                

    float x = (2.0f * (float)xpos) / (float)width - 1.0f;                
    float y = (2.0f * (float)ypos) / (float)height - 1.0f;

    Projection invProj;                
    invertOrtho(&proj, &invProj);                

    float worldX = x * invProj.mat[0] + y * invProj.mat[4] + invProj.mat[12];                
    float worldY = x * invProj.mat[1] + y * invProj.mat[5] + invProj.mat[13];                

    int cellX = (int)((worldX - Grid.left) / ((Grid.right - Grid.left) / textureWidth));                
    int cellY = (int)((worldY - Grid.bot) / ((Grid.top - Grid.bot) / textureHeight));                

    if (cellX >= 0 && cellX < textureWidth && cellY >= 0 && cellY < textureHeight) {                
        int index = cellY * textureWidth + cellX;                
        Field.current[index] = action;               
    }
}

int isFreeze(){
    extern char freeze;
    extern double lastUpdateTime;
    extern double updateInterval;
    double currentTime = glfwGetTime();
    if(freeze && currentTime - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime;
        return 0;
    }
    return 1;
}

void setWindowImage(GLFWwindow* window){
    GLFWimage images[3];
    int channels;

    images[0].pixels = stbi_load("img/icon16.png", &images[0].width, &images[0].height, &channels, 4);
    images[1].pixels = stbi_load("img/icon32.png", &images[1].width, &images[1].height, &channels, 4);
    images[2].pixels = stbi_load("img/icon48.png", &images[2].width, &images[2].height, &channels, 4);

    glfwSetWindowIcon(window, 3, images);

    stbi_image_free(images[0].pixels);
    stbi_image_free(images[1].pixels);
    stbi_image_free(images[2].pixels);

}

void setCallback(GLFWwindow* window){
    setWindowImage(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, buffersizeCallback);
    buffersizeCallback(window, 1024, 768);
    glfwSetScrollCallback(window, scrollCallback);
    glClearColor(0.0f, 0.749f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

