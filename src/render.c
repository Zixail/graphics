#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include "render.h"
#include "transform.h"
#include "life.h"

struct quad Quad;
struct grid Grid;
struct shader mShader;

extern Projection proj;

GLuint maskTex;
unsigned char* textureData = NULL;
int textureWidth, textureHeight;

float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};


char* readShader(const char* path){
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char * data = malloc(size + 1);
    size_t read = fread(data, sizeof(char), size, f);
    data[read] = '\0';

    fclose(f);
    return data;
}

void gridInit(){
    textureWidth = Field.width;
    textureHeight = Field.height;

    Grid.top =  1.0f * textureHeight / 20;
    Grid.bot = -1.0f * textureHeight / 20;
    Grid.right =  1.0f * textureWidth / 20;
    Grid.left =  -1.0f * textureWidth / 20;

    Grid.count = (Field.width + Field.height) + 2;
    float* lines = calloc(4 * Grid.count, sizeof(float));
    float dx = (Grid.right - Grid.left) / textureWidth;
    float dy = (Grid.top - Grid.bot) / textureHeight;
    
    for (int i = 0; i <= Field.width; ++i){
        lines[i * 4 + 0] = Grid.left + i * dx;
        lines[i * 4 + 1] = Grid.top;
        lines[i * 4 + 2] = Grid.left + i * dx;
        lines[i * 4 + 3] = Grid.bot;
    }

    for (int i = 0; i <= Field.height; ++i){
        int base = Field.width + i + 1;
        lines[base * 4 + 0] = Grid.left;
        lines[base * 4 + 1] = Grid.bot + i * dy;
        lines[base * 4 + 2] = Grid.right;
        lines[base * 4 + 3] = Grid.bot + i * dy;
    }

    glGenVertexArrays(1, &Grid.VAO);
    glGenBuffers(1, &Grid.VBO);

    glBindVertexArray(Grid.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, Grid.VBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * Grid.count * sizeof(float), (void*)lines, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    free(lines);
}

void quadInit(){
    Quad.vert[0]  = Grid.left;  Quad.vert[1]  = Grid.bot; Quad.vert[2]  = 0.0f; Quad.vert[3]  = 1.0f;
    Quad.vert[4]  = Grid.right; Quad.vert[5]  = Grid.bot; Quad.vert[6]  = 1.0f; Quad.vert[7]  = 1.0f;
    Quad.vert[8]  = Grid.right; Quad.vert[9]  = Grid.top; Quad.vert[10] = 1.0f; Quad.vert[11] = 0.0f;
    Quad.vert[12] = Grid.left;  Quad.vert[13] = Grid.top; Quad.vert[14] = 0.0f; Quad.vert[15] = 0.0f;

    Quad.ind[0] = 0; Quad.ind[1] = 1; Quad.ind[2] = 2;
    Quad.ind[3] = 0; Quad.ind[4] = 2; Quad.ind[5] = 3;

    textureData = malloc(textureWidth * textureHeight * 4); 

    glGenTextures(1, &maskTex);
    glBindTexture(GL_TEXTURE_2D, maskTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenVertexArrays(1, &Quad.VAO);
    glGenBuffers(1, &Quad.VBO);
    glGenBuffers(1, &Quad.EBO);

    glBindVertexArray(Quad.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, Quad.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Quad.vert), Quad.vert, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Quad.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Quad.ind), Quad.ind, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void programCreate(){
    char* vertexShaderSource = readShader("shaders/life.vert");
    char* fragmentShaderSource = readShader("shaders/life.frag");

    if (!vertexShaderSource || !fragmentShaderSource) {
        free(vertexShaderSource);
        free(fragmentShaderSource);
        return;
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexShaderSourcePtr = vertexShaderSource;
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentShaderSourcePtr = fragmentShaderSource;
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, NULL);
    glCompileShader(fragmentShader);

    mShader.program = glCreateProgram();
    glAttachShader(mShader.program, vertexShader);
    glAttachShader(mShader.program, fragmentShader);
    glLinkProgram(mShader.program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    free(vertexShaderSource);
    free(fragmentShaderSource);

    mShader.mask = glGetUniformLocation(mShader.program, "mask");
    mShader.projection = glGetUniformLocation(mShader.program, "projection");
    mShader.model = glGetUniformLocation(mShader.program, "model");
    mShader.view = glGetUniformLocation(mShader.program, "view");
    mShader.mode = glGetUniformLocation(mShader.program, "mode");
}

void updateTexture(){
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

}

void updateMat(){
    glUniform1i(mShader.mask, 0);
    glUniformMatrix4fv(mShader.projection, 1, GL_FALSE, proj.mat);
    glUniformMatrix4fv(mShader.model, 1, GL_FALSE, identity);
    glUniformMatrix4fv(mShader.view, 1, GL_FALSE, identity);
}

void renderGrid(){
    glUniform1i(mShader.mode, 1);
    glBindVertexArray(Grid.VAO);
    glDrawArrays(GL_LINES, 0, Grid.count * 2);
}

void renderTexture(){
    glUniform1i(mShader.mode, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, maskTex);
    glBindVertexArray(Quad.VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void cleanupRenderResources(){
    if (maskTex != 0) {
        glDeleteTextures(1, &maskTex);
        maskTex = 0;
    }

    if (Grid.VBO != 0) {
        glDeleteBuffers(1, &Grid.VBO);
        Grid.VBO = 0;
    }
    if (Grid.VAO != 0) {
        glDeleteVertexArrays(1, &Grid.VAO);
        Grid.VAO = 0;
    }

    if (Quad.EBO != 0) {
        glDeleteBuffers(1, &Quad.EBO);
        Quad.EBO = 0;
    }
    if (Quad.VBO != 0) {
        glDeleteBuffers(1, &Quad.VBO);
        Quad.VBO = 0;
    }
    if (Quad.VAO != 0) {
        glDeleteVertexArrays(1, &Quad.VAO);
        Quad.VAO = 0;
    }

    free(textureData);
    textureData = NULL;
}