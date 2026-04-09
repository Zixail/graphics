#ifndef CALLBACKS_H
#define CALLBACKS_H
#include <GLFW/glfw3.h>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mod);
void buffersizeCallback(GLFWwindow* window, int width, int height);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
int isFreeze();
void setCallback(GLFWwindow* window);
void setWindowImage(GLFWwindow* window);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
void processMouseClick(GLFWwindow* window, int action);
void updateCamera(GLFWwindow* window, float deltaTime);

#endif