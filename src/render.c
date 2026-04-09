#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include "render.h"
#include "transform.h"
#include "life.h"

struct render_state Render = {0};

float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

static int compileShaderOrLog(GLuint shader, const char* stageName) {
    GLint success = 0;
    char infoLog[1024];

    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "%s shader compile error: %s\n", stageName, infoLog);
        return 0;
    }

    return 1;
}

static int linkProgramOrLog(GLuint program) {
    GLint success = 0;
    char infoLog[1024];

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Program link error: %s\n", infoLog);
        return 0;
    }

    return 1;
}


char* readShader(const char* path){
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open shader file: %s\n", path);
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }

    long fileSize = ftell(f);
    if (fileSize < 0) {
        fclose(f);
        return NULL;
    }

    rewind(f);

    size_t size = (size_t)fileSize;
    char* data = malloc(size + 1);
    if (!data) {
        fclose(f);
        return NULL;
    }

    size_t bytesRead = fread(data, sizeof(char), size, f);
    if (bytesRead < size && ferror(f)) {
        free(data);
        fclose(f);
        return NULL;
    }

    data[bytesRead] = '\0';

    fclose(f);
    return data;
}

void gridInit(){
    Render.textureWidth = Life.width;
    Render.textureHeight = Life.height;

    if (Render.textureWidth <= 0 || Render.textureHeight <= 0) {
        return;
    }

    Render.grid.top =  1.0f * Render.textureHeight / 20;
    Render.grid.bot = -1.0f * Render.textureHeight / 20;
    Render.grid.right =  1.0f * Render.textureWidth / 20;
    Render.grid.left =  -1.0f * Render.textureWidth / 20;

    Render.grid.count = (Life.width + Life.height) + 2;
    float* lines = calloc(4 * Render.grid.count, sizeof(float));
    if (!lines) {
        return;
    }

    float dx = (Render.grid.right - Render.grid.left) / Render.textureWidth;
    float dy = (Render.grid.top - Render.grid.bot) / Render.textureHeight;
    
    for (int i = 0; i <= Life.width; ++i){
        lines[i * 4 + 0] = Render.grid.left + i * dx;
        lines[i * 4 + 1] = Render.grid.top;
        lines[i * 4 + 2] = Render.grid.left + i * dx;
        lines[i * 4 + 3] = Render.grid.bot;
    }

    for (int i = 0; i <= Life.height; ++i){
        int base = Life.width + i + 1;
        lines[base * 4 + 0] = Render.grid.left;
        lines[base * 4 + 1] = Render.grid.bot + i * dy;
        lines[base * 4 + 2] = Render.grid.right;
        lines[base * 4 + 3] = Render.grid.bot + i * dy;
    }

    glGenVertexArrays(1, &Render.grid.VAO);
    glGenBuffers(1, &Render.grid.VBO);

    glBindVertexArray(Render.grid.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, Render.grid.VBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * Render.grid.count * sizeof(float), (void*)lines, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    free(lines);
}

void quadInit(){
    Render.quad.vert[0]  = Render.grid.left;  Render.quad.vert[1]  = Render.grid.bot; Render.quad.vert[2]  = 0.0f; Render.quad.vert[3]  = 1.0f;
    Render.quad.vert[4]  = Render.grid.right; Render.quad.vert[5]  = Render.grid.bot; Render.quad.vert[6]  = 1.0f; Render.quad.vert[7]  = 1.0f;
    Render.quad.vert[8]  = Render.grid.right; Render.quad.vert[9]  = Render.grid.top; Render.quad.vert[10] = 1.0f; Render.quad.vert[11] = 0.0f;
    Render.quad.vert[12] = Render.grid.left;  Render.quad.vert[13] = Render.grid.top; Render.quad.vert[14] = 0.0f; Render.quad.vert[15] = 0.0f;

    Render.quad.ind[0] = 0; Render.quad.ind[1] = 1; Render.quad.ind[2] = 2;
    Render.quad.ind[3] = 0; Render.quad.ind[4] = 2; Render.quad.ind[5] = 3;

    if (Render.textureWidth <= 0 || Render.textureHeight <= 0) {
        return;
    }

    Render.textureData = malloc((size_t)Render.textureWidth * (size_t)Render.textureHeight * 4);
    if (!Render.textureData) {
        fprintf(stderr, "Failed to allocate texture buffer\n");
        return;
    }

    glGenTextures(1, &Render.maskTex);
    glBindTexture(GL_TEXTURE_2D, Render.maskTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Render.textureWidth, Render.textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenVertexArrays(1, &Render.quad.VAO);
    glGenBuffers(1, &Render.quad.VBO);
    glGenBuffers(1, &Render.quad.EBO);

    glBindVertexArray(Render.quad.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, Render.quad.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Render.quad.vert), Render.quad.vert, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Render.quad.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Render.quad.ind), Render.quad.ind, GL_STATIC_DRAW);

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
        fprintf(stderr, "Failed to load shader sources\n");
        free(vertexShaderSource);
        free(fragmentShaderSource);
        return;
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexShaderSourcePtr = vertexShaderSource;
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, NULL);
    if (!compileShaderOrLog(vertexShader, "Vertex")) {
        glDeleteShader(vertexShader);
        free(vertexShaderSource);
        free(fragmentShaderSource);
        return;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentShaderSourcePtr = fragmentShaderSource;
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, NULL);
    if (!compileShaderOrLog(fragmentShader, "Fragment")) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        free(vertexShaderSource);
        free(fragmentShaderSource);
        return;
    }

    Render.shader.program = glCreateProgram();
    if (Render.shader.program == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        free(vertexShaderSource);
        free(fragmentShaderSource);
        return;
    }

    glAttachShader(Render.shader.program, vertexShader);
    glAttachShader(Render.shader.program, fragmentShader);
    if (!linkProgramOrLog(Render.shader.program)) {
        glDeleteProgram(Render.shader.program);
        Render.shader.program = 0;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        free(vertexShaderSource);
        free(fragmentShaderSource);
        return;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    free(vertexShaderSource);
    free(fragmentShaderSource);

    Render.shader.mask = glGetUniformLocation(Render.shader.program, "mask");
    Render.shader.projection = glGetUniformLocation(Render.shader.program, "projection");
    Render.shader.model = glGetUniformLocation(Render.shader.program, "model");
    Render.shader.view = glGetUniformLocation(Render.shader.program, "view");
    Render.shader.mode = glGetUniformLocation(Render.shader.program, "mode");
}

void updateTexture(){
    if (!Render.textureData || Render.textureWidth <= 0 || Render.textureHeight <= 0) {
        return;
    }

    for(int i = 0; i < Render.textureHeight; i++) {
        for(int j = 0; j < Render.textureWidth; j++) {
            int fieldIdx = i * Render.textureWidth + j;
            unsigned char cell = Life.current[fieldIdx];
            
            int texIdx = (i * Render.textureWidth + j) * 4;
            if (cell == 0){
                Render.textureData[texIdx + 0] = 255; 
                Render.textureData[texIdx + 1] = 255;
                Render.textureData[texIdx + 2] = 255; 
                Render.textureData[texIdx + 3] = 0; 
            }
            else{
                Render.textureData[texIdx + 0] = 0;  
                Render.textureData[texIdx + 1] = 0;  
                Render.textureData[texIdx + 2] = 0;   
                Render.textureData[texIdx + 3] = 255; 
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, Render.maskTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Render.textureWidth, Render.textureHeight, GL_RGBA, GL_UNSIGNED_BYTE, Render.textureData);

}

void updateMat(){
    Projection* proj = getProjection();
    glUniform1i(Render.shader.mask, 0);
    glUniformMatrix4fv(Render.shader.projection, 1, GL_FALSE, proj->mat);
    glUniformMatrix4fv(Render.shader.model, 1, GL_FALSE, identity);
    glUniformMatrix4fv(Render.shader.view, 1, GL_FALSE, identity);
}

void renderGrid(){
    glUniform1i(Render.shader.mode, 1);
    glBindVertexArray(Render.grid.VAO);
    glDrawArrays(GL_LINES, 0, Render.grid.count * 2);
}

void renderTexture(){
    glUniform1i(Render.shader.mode, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Render.maskTex);
    glBindVertexArray(Render.quad.VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void cleanupRenderResources(){
    if (Render.maskTex != 0) {
        glDeleteTextures(1, &Render.maskTex);
        Render.maskTex = 0;
    }

    if (Render.grid.VBO != 0) {
        glDeleteBuffers(1, &Render.grid.VBO);
        Render.grid.VBO = 0;
    }
    if (Render.grid.VAO != 0) {
        glDeleteVertexArrays(1, &Render.grid.VAO);
        Render.grid.VAO = 0;
    }

    if (Render.quad.EBO != 0) {
        glDeleteBuffers(1, &Render.quad.EBO);
        Render.quad.EBO = 0;
    }
    if (Render.quad.VBO != 0) {
        glDeleteBuffers(1, &Render.quad.VBO);
        Render.quad.VBO = 0;
    }
    if (Render.quad.VAO != 0) {
        glDeleteVertexArrays(1, &Render.quad.VAO);
        Render.quad.VAO = 0;
    }

    free(Render.textureData);
    Render.textureData = NULL;
}
