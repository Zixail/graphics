#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>

#include "transform.h"
#include "utils.h"

float projection[16];
float gZoom = 1.0f;


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, 1);
    }
}

void buffersize_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);

    extern float projection[];
    extern float gZoom;

    float aspect = (float)width / (float)height;

    float bottom = -1.0f * gZoom;
    float top = 1.0f * gZoom;

    float left = -aspect * gZoom;
    float right = aspect * gZoom;

    float near = 1.0f;
    float far = -1.0f;

    makeOrtho(left, right, bottom, top, near, far, projection);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    extern float gZoom;
    float zoomSpeed = 0.1f;
    gZoom -= yoffset * zoomSpeed;
    if(gZoom < 0.1f) gZoom = 0.1f;
    if(gZoom > 10.0f) gZoom = 10.0f;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    buffersize_callback(window, width, height);
}

int main(void){

    if(!glfwInit()){
        printf("glfw error!\n");
        return -1;
    }

    int width = 1024;
    int height = 768;

    GLFWwindow* window = glfwCreateWindow(width, height, "Test", NULL, NULL);

    if(!window){
        printf("window error!\n");
        glfwTerminate();
        return -2;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("glad error!\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -2;
    }

    glfwSwapInterval(1);
    glViewport(0, 0, 1024, 768);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window ,buffersize_callback);
    glfwSetScrollCallback(window, scroll_callback);
    buffersize_callback(window, 1024, 768);

    
    float triangles[] = {
        0.0f, 0.3f,     0.5f, 0.7f, 0.2f,
        -0.3f, -0.3f,     0.7f, 0.2f, 0.5f,
        0.3f, -0.3f,     0.2f, 0.5f, 0.7f,
    };

    GLuint trVAO, trVBO;

    glGenVertexArrays(1, &trVAO);
    glGenBuffers(1, &trVBO);

    glBindVertexArray(trVAO);

    glBindBuffer(GL_ARRAY_BUFFER, trVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    const char* vertexShaderSource = loadFile("shaders/TransWorldTriangle5.vert");
    const char* fragmentShaderSource = loadFile("shaders/SimpleTriangle5.frag");

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    Transform triangleT, lineT, viewT;
    transformInit(&triangleT);
    transformInit(&lineT);

    transformInit(&viewT);
    
    extern float projection[];

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");


    float lines[] = {
        -0.75f, 0.2f,     0.4f, 0.3f, 0.7f,
        0.4f, -0.25f,     0.1f, 0.5f, 0.2f,
    };



    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewT.model);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
        
        triangleT.rotationZ = glfwGetTime();
        triangleT.x = cosf(glfwGetTime()) / 5;
        triangleT.y = sinf(glfwGetTime()) / 5;

        transformUpdateModel(&triangleT);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, triangleT.model);

        glBindVertexArray(trVAO);
        

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}