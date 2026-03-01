#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "transform.h"
#include "render.h"
#include "life.h"
#include "input.h"

extern struct quad Quad;
extern struct _grid Grid;
extern struct shader mShader;

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

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("glad error");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -3;
    }

    setCallback(window);

    initField(50, 50);
    loadSample();
    gridInit();
    quadInit();
    programCreate();

    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();                
        glClear(GL_COLOR_BUFFER_BIT);      

        if(!isFreeze()) {
            updateField();  
        }

        glUseProgram(mShader.program);
        updateMat();
        updateTexture();

        renderGrid();
        renderTexture();

        glfwSwapBuffers(window);             
    }

    glDeleteProgram(mShader.program);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}