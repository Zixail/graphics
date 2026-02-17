#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "utils.h"

void buffersize_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
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

    GLFWwindow* window = glfwCreateWindow(800, 600, "Test", NULL, NULL);

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

    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, buffersize_callback);

    glViewport(0, 0, 800, 600);

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

    const char* vertexShaderSource = loadFile("shaders/simpleTriangle5.vert");

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

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
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