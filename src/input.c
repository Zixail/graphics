#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "transform.h"
#include "input.h"
#include "render.h"
#include "life.h"

static float zoom = 1.0f;
static float cameraX = 0.0f;
static float cameraY = 0.0f;
static double lastUpdateTime = 0.0;
static double updateInterval = 0.1;
static char freeze = 0;
static char isFullscreen = 0;
static int windowedX, windowedY, windowedWidth, windowedHeight;

static float clampFloat(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static void clampCameraToGrid(float aspect) {
    const struct grid* grid = &Render.grid;
    const float edgePaddingRatio = 0.12f;

    float padX = (grid->right - grid->left) * edgePaddingRatio;
    float padY = (grid->top - grid->bot) * edgePaddingRatio;

    float extendedLeft = grid->left - padX;
    float extendedRight = grid->right + padX;
    float extendedBot = grid->bot - padY;
    float extendedTop = grid->top + padY;

    float halfWidth = aspect * zoom;
    float halfHeight = zoom;

    float minCameraX = extendedLeft + halfWidth;
    float maxCameraX = extendedRight - halfWidth;
    float minCameraY = extendedBot + halfHeight;
    float maxCameraY = extendedTop - halfHeight;

    if (minCameraX > maxCameraX) {
        cameraX = 0.5f * (grid->left + grid->right);
    } else {
        cameraX = clampFloat(cameraX, minCameraX, maxCameraX);
    }

    if (minCameraY > maxCameraY) {
        cameraY = 0.5f * (grid->bot + grid->top);
    } else {
        cameraY = clampFloat(cameraY, minCameraY, maxCameraY);
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mod){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, 1);
    }
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS){
        freeze = !freeze;
    }
    if(key == GLFW_KEY_Z && action == GLFW_PRESS){
        updateInterval *= (double)2;
        if (updateInterval > 1.6f) updateInterval = 1.6f;
    }
    if(key == GLFW_KEY_X && action == GLFW_PRESS){
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

    if (height == 0) {
        return;
    }

    float aspect = (float)width / (float)height;
    clampCameraToGrid(aspect);

    float bottom = cameraY - zoom;
    float top = cameraY + zoom;

    float left = cameraX - aspect * zoom;
    float right = cameraX + aspect * zoom;

    float near = -1.0f;
    float far = 1.0f;

    Projection* proj = getProjection();
    makeOrtho(left, right, bottom, top, near, far, proj);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    float zoomSpeed = 0.1f;
    zoom -= (float)yoffset *zoomSpeed;
    if (zoom < 0.1f) zoom = 0.1f;
    if (zoom > 10.0f) zoom = 10.0f;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    buffersizeCallback(window, width, height);
}

void updateCamera(GLFWwindow* window, float deltaTime) {
    float dirX = 0.0f;
    float dirY = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dirY += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dirY -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dirX += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dirX -= 1.0f;

    if (dirX == 0.0f && dirY == 0.0f) {
        return;
    }

    if (dirX != 0.0f && dirY != 0.0f) {
        const float invSqrt2 = 0.70710678f;
        dirX *= invSqrt2;
        dirY *= invSqrt2;
    }

    float moveSpeed = 2.0f * zoom;
    cameraX += dirX * moveSpeed * deltaTime;
    cameraY += dirY * moveSpeed * deltaTime;

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
    Projection* proj = getProjection();
    const struct grid* grid = &Render.grid;
    int textureWidth = Render.textureWidth;
    int textureHeight = Render.textureHeight;

    double xpos, ypos;                
    glfwGetCursorPos(window, &xpos, &ypos);                

    int windowWidth, windowHeight;
    int framebufferWidth, framebufferHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    if (windowWidth <= 0 || windowHeight <= 0 || framebufferWidth <= 0 || framebufferHeight <= 0) {
        return;
    }

    float scaledX = (float)xpos * ((float)framebufferWidth / (float)windowWidth);
    float scaledY = (float)ypos * ((float)framebufferHeight / (float)windowHeight);

    float x = (2.0f * scaledX) / (float)framebufferWidth - 1.0f;
    float y = 1.0f - (2.0f * scaledY) / (float)framebufferHeight;

    Projection invProj;                
    invertOrtho(proj, &invProj);                

    float worldX = x * invProj.mat[0] + y * invProj.mat[4] + invProj.mat[12];                
    float worldY = x * invProj.mat[1] + y * invProj.mat[5] + invProj.mat[13];                

    int cellX = (int)((worldX - grid->left) / ((grid->right - grid->left) / textureWidth));
    int cellY = (int)((grid->top - worldY) / ((grid->top - grid->bot) / textureHeight));

    if (cellX >= 0 && cellX < textureWidth && cellY >= 0 && cellY < textureHeight) {                
        int index = cellY * textureWidth + cellX;                
        Field.current[index] = action;               
    }
}

int isFreeze(){
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

