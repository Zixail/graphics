#ifndef UTILS_H
#define UTILS_H

struct _grid{
    float top, bot;
    float right, left;
    GLuint VAO, VBO;
    int count;
};

struct _quad{
    float vert[16];
    unsigned int ind[6];
    GLuint VAO, VBO, EBO;
};

struct shader{
    GLint program;
    GLint mask;                
    GLint projection;                
    GLint model;                
    GLint view;                
    GLint mode;
};

char* readShader(const char* path);
void gridInit();
void quadInit();
void programCreate();
void updateMat();
void updateTexture();
void renderGrid();
void renderTexture();

#endif