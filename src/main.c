#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "transform.h"
#include "render.h"
#include "life.h"
#include "input.h"

enum menu_mode {
    MENU_MODE_MAIN = 0,
    MENU_MODE_SIZE_INPUT = 1
};

static int gMenuMode = MENU_MODE_MAIN;
static int gActiveField = 0;
static int gSizeConfirmRequested = 0;
static char gSizeX[8] = "50";
static char gSizeY[8] = "50";

static void appendDigit(char* buffer, size_t capacity, char digit) {
    size_t len = strlen(buffer);
    if (len >= 3) {
        return;  // limit to 3 digits
    }
    if (len + 1 < capacity) {
        if (len == 1 && buffer[0] == '0') {
            buffer[0] = digit;
            return;
        }
        buffer[len] = digit;
        buffer[len + 1] = '\0';
    }
}

static void removeLastDigit(char* buffer) {
    size_t len = strlen(buffer);
    if (len > 0) {
        buffer[len - 1] = '\0';
    }
    if (buffer[0] == '\0') {
        buffer[0] = '0';
        buffer[1] = '\0';
    }
}

static void updateMenuTitle(GLFWwindow* window) {
    glfwSetWindowTitle(window, "Game of Life");
}

static void menuCharCallback(GLFWwindow* window, unsigned int codepoint) {
    (void)window;
    if (gMenuMode != MENU_MODE_SIZE_INPUT) {
        return;
    }
    if (codepoint >= '0' && codepoint <= '9') {
        if (gActiveField == 0) {
            appendDigit(gSizeX, sizeof(gSizeX), (char)codepoint);
        } else {
            appendDigit(gSizeY, sizeof(gSizeY), (char)codepoint);
        }
    }
}

static void menuKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }

    if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        Input.isFullscreen = !Input.isFullscreen;
        
        if (Input.isFullscreen) {
            glfwGetWindowPos(window, &Input.windowedX, &Input.windowedY);
            glfwGetWindowSize(window, &Input.windowedWidth, &Input.windowedHeight);
            
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else {
            glfwSetWindowMonitor(window, NULL, Input.windowedX, Input.windowedY, Input.windowedWidth, Input.windowedHeight, 0);
        }
        return;
    }

    if (gMenuMode == MENU_MODE_MAIN) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
        }
        return;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        gActiveField = 1 - gActiveField;
    } else if (key == GLFW_KEY_BACKSPACE) {
        if (gActiveField == 0) {
            removeLastDigit(gSizeX);
        } else {
            removeLastDigit(gSizeY);
        }
    } else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
        gSizeConfirmRequested = 1;
    } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        gMenuMode = MENU_MODE_MAIN;
    }
}

static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

int main(void){

    if(!glfwInit()){
        printf("glfw error");
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Game of Life", NULL, NULL);
    if(!window){
        printf("window error");
        glfwTerminate();
        return -2;
    }

    setWindowImage(window);

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("glad error");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -3;
    }

    glfwSwapInterval(1);
    glClearColor(0.0f, 0.749f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, menuKeyCallback);
    glfwSetCharCallback(window, menuCharCallback);

    programCreate();
    if (Render.shader.program == 0) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -4;
    }

    if (!menuInit()) {
        glDeleteProgram(Render.shader.program);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -5;
    }

    double lastFrameTime = glfwGetTime();
    int gameStarted = 0;
    int previousLeftMouseState = GLFW_RELEASE;

    while(!glfwWindowShouldClose(window)){
        double currentFrameTime = glfwGetTime();
        float deltaTime = (float)(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        glfwPollEvents();                
        glClear(GL_COLOR_BUFFER_BIT);      
        updateMenuTitle(window);

        if (!gameStarted) {
            renderMenu(gSizeX, gSizeY, gMenuMode == MENU_MODE_SIZE_INPUT, gActiveField);

            int currentLeftMouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            if (currentLeftMouseState == GLFW_PRESS && previousLeftMouseState == GLFW_RELEASE) {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);

                int width, height;
                glfwGetWindowSize(window, &width, &height);
                if (width > 0 && height > 0) {
                    float ndcX = (2.0f * (float)xpos) / (float)width - 1.0f;
                    float ndcY = 1.0f - (2.0f * (float)ypos) / (float)height;
                    int hit = menuHitTestNdc(ndcX, ndcY);

                    if (hit == 1) {
                        if (gMenuMode == MENU_MODE_MAIN) {
                            gMenuMode = MENU_MODE_SIZE_INPUT;
                            gActiveField = 0;
                        } else {
                            gSizeConfirmRequested = 1;
                        }
                    } else if (hit == 2) {
                        if (gMenuMode == MENU_MODE_MAIN) {
                            glfwSetWindowShouldClose(window, 1);
                        } else {
                            gMenuMode = MENU_MODE_MAIN;
                        }
                    } else if (hit == 3 && gMenuMode == MENU_MODE_SIZE_INPUT) {
                        gActiveField = 0;
                    } else if (hit == 4 && gMenuMode == MENU_MODE_SIZE_INPUT) {
                        gActiveField = 1;
                    }
                }
            }
            previousLeftMouseState = currentLeftMouseState;

            if (gSizeConfirmRequested) {
                int fieldWidth = atoi(gSizeX);
                int fieldHeight = atoi(gSizeY);
                if (fieldWidth < 5) fieldWidth = 5;
                if (fieldHeight < 5) fieldHeight = 5;
                if (fieldWidth > 500) fieldWidth = 500;
                if (fieldHeight > 500) fieldHeight = 500;

                initField(fieldWidth, fieldHeight);
                loadSample();
                gridInit();
                quadInit();
                setCallback(window);
                glfwSetWindowTitle(window, "Game of Life");
                cleanupMenuResources();
                gameStarted = 1;
                gSizeConfirmRequested = 0;
            }
        } else {
            updateCamera(window, deltaTime);

            if(!isFreeze()) {
                updateField();  
            }

            glUseProgram(Render.shader.program);
            updateMat();
            updateTexture();

            renderGrid();
            renderTexture();
        }

        glfwSwapBuffers(window);             
    }

    cleanupMenuResources();
    cleanupRenderResources();
    freeField();
    glDeleteProgram(Render.shader.program);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}