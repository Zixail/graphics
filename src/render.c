#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>
#include "render.h"
#include "transform.h"
#include "life.h"

struct render_state Render = {0};

float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

static const float startButtonLeft = -0.45f;
static const float startButtonRight = 0.45f;
static const float startButtonBottom = 0.10f;
static const float startButtonTop = 0.38f;

static const float exitButtonLeft = -0.45f;
static const float exitButtonRight = 0.45f;
static const float exitButtonBottom = -0.32f;
static const float exitButtonTop = -0.04f;

static const float inputBoxXLeft = -0.50f;
static const float inputBoxXRight = 0.0f;
static const float inputBoxYLeft = 0.0f;
static const float inputBoxYRight = 0.50f;
static const float inputBoxBottom = -0.75f;
static const float inputBoxTop = -0.48f;

static const int menuFontAtlasWidth = 1024;
static const int menuFontAtlasHeight = 1024;
static const float menuFontPixelHeight = 48.0f;
static stbtt_packedchar menuFontChars[96];

static void getMenuProjectionMatrix(float* outMat) {
    int width = 1024, height = 768;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
    if (width <= 0 || height <= 0) {
        width = 1024;
        height = 768;
    }

    float aspect = (float)width / (float)height;
    float left = -aspect;
    float right = aspect;
    float bottom = -1.0f;
    float top = 1.0f;

    float rl = right - left;
    float tb = top - bottom;
    float fn = 1.0f - (-1.0f);

    outMat[0] = 2.0f / rl;              outMat[1] = 0.0f;                   outMat[2] = 0.0f;                 outMat[3] = 0.0f;
    outMat[4] = 0.0f;                   outMat[5] = 2.0f / tb;              outMat[6] = 0.0f;                 outMat[7] = 0.0f;
    outMat[8] = 0.0f;                   outMat[9] = 0.0f;                   outMat[10] = -2.0f / fn;          outMat[11] = 0.0f;
    outMat[12] = -(right + left) / rl;  outMat[13] = -(top + bottom) / tb;  outMat[14] = -(1.0f - 1.0f) / fn; outMat[15] = 1.0f;
}

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

static GLuint loadTextureFromFile(const char* path) {
    int width, height, channels;
    unsigned char* pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!pixels) {
        fprintf(stderr, "Failed to load texture: %s\n", path);
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(pixels);
    return tex;
}

static unsigned char* readBinaryFile(const char* path, size_t* outSize) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open file: %s\n", path);
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

    unsigned char* data = malloc((size_t)fileSize + 1);
    if (!data) {
        fclose(f);
        return NULL;
    }

    size_t bytesRead = fread(data, 1, (size_t)fileSize, f);
    if (bytesRead != (size_t)fileSize) {
        free(data);
        fclose(f);
        return NULL;
    }

    fclose(f);
    if (outSize) {
        *outSize = bytesRead;
    }
    return data;
}

static int fontInit(void) {
    if (Render.menuFontReady) {
        return 1;
    }

    size_t fontSize = 0;
    unsigned char* fontData = readBinaryFile("img/impact.ttf", &fontSize);
    if (!fontData) {
        return 0;
    }

    unsigned char* bitmap = calloc((size_t)menuFontAtlasWidth * (size_t)menuFontAtlasHeight, sizeof(unsigned char));
    if (!bitmap) {
        free(fontData);
        return 0;
    }

    stbtt_pack_context packContext;
    if (!stbtt_PackBegin(&packContext, bitmap, menuFontAtlasWidth, menuFontAtlasHeight, 0, 1, NULL)) {
        free(bitmap);
        free(fontData);
        return 0;
    }

    stbtt_PackSetOversampling(&packContext, 3, 3);

    stbtt_pack_range range;
    memset(&range, 0, sizeof(range));
    range.first_unicode_codepoint_in_range = 32;
    range.num_chars = 96;
    range.font_size = menuFontPixelHeight;
    range.chardata_for_range = menuFontChars;

    int packResult = stbtt_PackFontRanges(&packContext, fontData, 0, &range, 1);
    stbtt_PackEnd(&packContext);
    free(fontData);

    if (!packResult) {
        free(bitmap);
        return 0;
    }

    unsigned char* rgba = malloc((size_t)menuFontAtlasWidth * (size_t)menuFontAtlasHeight * 4);
    if (!rgba) {
        free(bitmap);
        return 0;
    }

    for (int i = 0; i < menuFontAtlasWidth * menuFontAtlasHeight; ++i) {
        unsigned char alpha = bitmap[i];
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 255;
        rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = alpha;
    }

    free(bitmap);

    glGenTextures(1, &Render.menuFontTex);
    glBindTexture(GL_TEXTURE_2D, Render.menuFontTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, menuFontAtlasWidth, menuFontAtlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);

    free(rgba);
    Render.menuFontReady = 1;
    return 1;
}

static void drawColoredQuad(float left, float right, float bottom, float top, float r, float g, float b, float a) {
    float vertices[16] = {
        left,  bottom, 0.0f, 0.0f,
        right, bottom, 0.0f, 0.0f,
        right, top,    0.0f, 0.0f,
        left,  top,    0.0f, 0.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, Render.menuVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glVertexAttrib4f(2, r, g, b, a);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void drawTexturedQuad(float left, float right, float bottom, float top, float u0, float v0, float u1, float v1) {
    float vertices[16] = {
        left,  bottom, u0, v1,
        right, bottom, u1, v1,
        right, top,    u1, v0,
        left,  top,    u0, v0
    };

    glBindBuffer(GL_ARRAY_BUFFER, Render.menuVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void drawMenuButton(GLuint texture, float left, float right, float bottom, float top) {
    glBindTexture(GL_TEXTURE_2D, texture);
    drawTexturedQuad(left, right, bottom, top, 0.0f, 0.0f, 1.0f, 1.0f);
}

static void measureTextBounds(const char* text, float* minX, float* maxX, float* minY, float* maxY) {
    float penX = 0.0f;
    float penY = 0.0f;
    *minX = 1e9f;
    *maxX = -1e9f;
    *minY = 1e9f;
    *maxY = -1e9f;

    for (const char* cursor = text; *cursor; ++cursor) {
        unsigned char ch = (unsigned char)*cursor;
        if (ch < 32 || ch > 126) {
            continue;
        }

        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(menuFontChars, menuFontAtlasWidth, menuFontAtlasHeight, ch - 32, &penX, &penY, &q, 1);

        if (q.x0 < *minX) *minX = q.x0;
        if (q.x1 > *maxX) *maxX = q.x1;
        if (q.y0 < *minY) *minY = q.y0;
        if (q.y1 > *maxY) *maxY = q.y1;
    }

    if (*minX > *maxX) {
        *minX = 0.0f;
        *maxX = 0.0f;
        *minY = 0.0f;
        *maxY = 0.0f;
    }
}

static void drawTextString(const char* text, float left, float right, float bottom, float top, float scale, float r, float g, float b, float a) {
    float minX, maxX, minY, maxY;
    measureTextBounds(text, &minX, &maxX, &minY, &maxY);
    float textWidth = (maxX - minX) * scale;
    float textHeight = (maxY - minY) * scale;
    float availableWidth = right - left;
    float availableHeight = top - bottom;

    float originX = left + (availableWidth - textWidth) * 0.5f - minX * scale;
    float originY = top - (availableHeight - textHeight) * 0.5f + minY * scale;

    glBindTexture(GL_TEXTURE_2D, Render.menuFontTex);
    glUniform1i(Render.shader.mode, 0);

    float penX = 0.0f;
    float penY = 0.0f;
    for (const char* cursor = text; *cursor; ++cursor) {
        unsigned char ch = (unsigned char)*cursor;
        if (ch < 32 || ch > 126) {
            continue;
        }

        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(menuFontChars, menuFontAtlasWidth, menuFontAtlasHeight, ch - 32, &penX, &penY, &q, 1);

        float vertices[16] = {
            originX + q.x0 * scale, originY - q.y1 * scale, q.s0, q.t1,
            originX + q.x1 * scale, originY - q.y1 * scale, q.s1, q.t1,
            originX + q.x1 * scale, originY - q.y0 * scale, q.s1, q.t0,
            originX + q.x0 * scale, originY - q.y0 * scale, q.s0, q.t0
        };

        glBindBuffer(GL_ARRAY_BUFFER, Render.menuVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glVertexAttrib4f(2, r, g, b, a);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
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

int menuInit() {
    if (Render.menuReady) {
        return 1;
    }

    Render.menuStartTex = loadTextureFromFile("img/start.png");
    Render.menuExitTex = loadTextureFromFile("img/exit.png");
    if (Render.menuStartTex == 0 || Render.menuExitTex == 0) {
        return 0;
    }

    if (!fontInit()) {
        return 0;
    }

    unsigned int indices[6] = {0, 1, 2, 0, 2, 3};
    glGenVertexArrays(1, &Render.menuVAO);
    glGenBuffers(1, &Render.menuVBO);
    glGenBuffers(1, &Render.menuEBO);

    glBindVertexArray(Render.menuVAO);
    glBindBuffer(GL_ARRAY_BUFFER, Render.menuVBO);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Render.menuEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    Render.menuReady = 1;
    return 1;
}

void renderMenu(const char* sizeX, const char* sizeY, int inputMode, int activeField) {
    if (!Render.menuReady || Render.shader.program == 0) {
        return;
    }

    float menuProjectionMat[16];
    getMenuProjectionMatrix(menuProjectionMat);

    glUseProgram(Render.shader.program);
    glUniform1i(Render.shader.mask, 0);
    glUniform1i(Render.shader.mode, 0);
    glUniformMatrix4fv(Render.shader.projection, 1, GL_FALSE, menuProjectionMat);
    glUniformMatrix4fv(Render.shader.model, 1, GL_FALSE, identity);
    glUniformMatrix4fv(Render.shader.view, 1, GL_FALSE, identity);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(Render.menuVAO);

    drawMenuButton(Render.menuStartTex, startButtonLeft, startButtonRight, startButtonBottom, startButtonTop);
    drawMenuButton(Render.menuExitTex, exitButtonLeft, exitButtonRight, exitButtonBottom, exitButtonTop);

    if (inputMode) {
        glUniform1i(Render.shader.mode, 1);
        drawColoredQuad(inputBoxXLeft, inputBoxXRight, inputBoxBottom, inputBoxTop, 0.08f, 0.10f, 0.16f, 1.0f);
        drawColoredQuad(inputBoxXLeft + 0.01f, inputBoxXRight - 0.01f, inputBoxBottom + 0.01f, inputBoxTop - 0.01f, activeField == 0 ? 0.93f : 0.86f, activeField == 0 ? 0.98f : 0.90f, activeField == 0 ? 1.00f : 0.95f, 1.0f);
        drawColoredQuad(inputBoxYLeft, inputBoxYRight, inputBoxBottom, inputBoxTop, 0.08f, 0.10f, 0.16f, 1.0f);
        drawColoredQuad(inputBoxYLeft + 0.01f, inputBoxYRight - 0.01f, inputBoxBottom + 0.01f, inputBoxTop - 0.01f, activeField == 1 ? 0.93f : 0.86f, activeField == 1 ? 0.98f : 0.90f, activeField == 1 ? 1.00f : 0.95f, 1.0f);

        glUniform1i(Render.shader.mode, 0);
        drawTextString(sizeX, inputBoxXLeft + 0.01f, inputBoxXRight - 0.01f, inputBoxBottom + 0.02f, inputBoxTop - 0.02f, 0.0070f, 0.0f, 0.0f, 0.0f, 1.0f);
        drawTextString(sizeY, inputBoxYLeft + 0.01f, inputBoxYRight - 0.01f, inputBoxBottom + 0.02f, inputBoxTop - 0.02f, 0.0070f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    glBindVertexArray(0);
}

int menuHitTestNdc(float x, float y) {
    if (x >= startButtonLeft && x <= startButtonRight && y >= startButtonBottom && y <= startButtonTop) {
        return 1;
    }
    if (x >= exitButtonLeft && x <= exitButtonRight && y >= exitButtonBottom && y <= exitButtonTop) {
        return 2;
    }
    if (x >= inputBoxXLeft && x <= inputBoxXRight && y >= inputBoxBottom && y <= inputBoxTop) {
        return 3;
    }
    if (x >= inputBoxYLeft && x <= inputBoxYRight && y >= inputBoxBottom && y <= inputBoxTop) {
        return 4;
    }
    return 0;
}

void cleanupMenuResources() {
    if (Render.menuFontTex != 0) {
        glDeleteTextures(1, &Render.menuFontTex);
        Render.menuFontTex = 0;
    }
    if (Render.menuStartTex != 0) {
        glDeleteTextures(1, &Render.menuStartTex);
        Render.menuStartTex = 0;
    }
    if (Render.menuExitTex != 0) {
        glDeleteTextures(1, &Render.menuExitTex);
        Render.menuExitTex = 0;
    }
    if (Render.menuEBO != 0) {
        glDeleteBuffers(1, &Render.menuEBO);
        Render.menuEBO = 0;
    }
    if (Render.menuVBO != 0) {
        glDeleteBuffers(1, &Render.menuVBO);
        Render.menuVBO = 0;
    }
    if (Render.menuVAO != 0) {
        glDeleteVertexArrays(1, &Render.menuVAO);
        Render.menuVAO = 0;
    }
    Render.menuFontReady = 0;
    Render.menuReady = 0;
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
