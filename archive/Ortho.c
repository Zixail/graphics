#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>

#include "transform.h"
#include "utils.h"

float projection[16];

void buffersize_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
    float aspect = (float)width / (float)height;

    float bottom = -1.0f;
    float top = 1.0f;

    float left = -aspect;
    float right = aspect;

    float near = -1.0f;
    float far = 1.0f;

    extern float projection[16];

    makeOrtho(left, right, bottom, top, near, far, projection);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, 1);
    }
    if(key == GLFW_KEY_R && action == GLFW_PRESS){
        glClearColor(1.0F, 0.0F, 0.0F, 1.0F);
    }
    if(key == GLFW_KEY_G && action == GLFW_PRESS){
        glClearColor(0.0F, 1.0F, 0.0F, 1.0F);
    }
    if(key == GLFW_KEY_B && action == GLFW_PRESS){
        glClearColor(0.0F, 0.0F, 1.0F, 1.0F);
    }
}

int main(void){

    if(!glfwInit()){
        printf("glfw initialization error");
        return -1;
    }

    int width = 1024;
    int height = 768;

    GLFWwindow* window = glfwCreateWindow(width, height, "Test", NULL, NULL);

    if(!window){
        printf("window create error");
        glfwTerminate();
        return -2;
    }

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("glad initizialization error");
        glfwTerminate();
        return -3;
    }
    
    float aspect = (float)width / (float)height;

    float bottom = -1.0f;
    float top = 1.0f;

    float left = -aspect;
    float right = aspect;

    float near = -1.0f;
    float far = 1.0f;


    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, buffersize_callback);

    glViewport(0, 0, width, height);

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    //x,  y      r,  g,  b
    float vertices[] = {
        0.0f, 0.5f,     1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
        0.0f, -0.5f,    0.0f, 0.0f, 1.0f,

        0.0f, 0.5f,     1.0f, 1.0f, 0.0f,
        0.5f, -0.5f,    1.0f, 0.0f, 1.0f,
        0.0f, -0.5f,     0.0f, 1.0f, 1.0f
    };

    GLuint VAO, VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    const char* vertexShaderSource = loadFile("shaders/TransWorldTriangle5.vert");

    const char* fragmentShaderSource = loadFile("shaders/simpleTriangle5.frag");
    
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

    Transform modelTr, viewTr;
    transformInit(&modelTr);
    transformInit(&viewTr);

    extern float projection[16];
    makeOrtho(left, right, bottom, top, near, far, projection);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();

        modelTr.rotationZ = glfwGetTime();
        modelTr.x = cosf(modelTr.rotationZ) / 2;
        modelTr.y = sinf(modelTr.rotationZ) / 2;

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        transformUpdateModel(&modelTr);
        transformUpdateModel(&viewTr);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelTr.model);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewTr.model);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}