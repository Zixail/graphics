#include <math.h>

typedef struct Projection {
    float mat[16];
} Projection;

typedef struct Transform {
    float x, y;
    float rotationZ;
    float sx, sy;
    float mat[16];
} Transform;

void transformInit(Transform* t){
    t->x = t->y = 0.0f;
    t->rotationZ = 0.0f;
    t->sx = t-> sy = 1.0f;
    float *m = t->mat;

    m[0] = 1.0f;  m[1] = 0.0f;   m[2] = 0.0f;   m[3] = 0.0f;
    m[4] = 0.0f;  m[5] = 1.0f;   m[6] = 0.0f;   m[7] = 0.0f;
    m[8] = 0.0f;  m[9] = 0.0f;   m[10] = 1.0f;  m[11] = 0.0f;
    m[12] = 0.0f; m[13] = 0.0f;  m[14] = 0.0f;  m[15] = 1.0f;
}

Projection proj;

void transformUpdateModel(Transform* t){
    float x = t->x;
    float y = t->y;

    float sx = t->sx;
    float sy = t->sy;

    float c = cosf(t->rotationZ);
    float s = sinf(t->rotationZ);

    float *m = t->mat;

    m[0] = c * sx;   m[1] = s * sx;   m[2] = 0.0f;   m[3] = 0.0f;
    m[4] = -s * sy;  m[5] = c * sy;   m[6] = 0.0f;   m[7] = 0.0f;
    m[8] = 0.0f;     m[9] = 0.0f;     m[10] = 1.0f;  m[11] = 0.0f;
    m[12] = x;       m[13] = y;       m[14] = 0.0f;     m[15] = 1.0f;
}

void makeOrtho(float left, float right, float bottom, float top, float near, float far, Projection* projection){
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;
    float* m = projection->mat;

    m[0] = 2.0f / rl;              m[1] = 0.0f;                   m[2] = 0.0f;                 m[3] = 0.0f;                     
    m[4] = 0.0f;                   m[5] = 2.0f / tb;              m[6] = 0.0f;                 m[7] = 0.0f;
    m[8] = 0.0f;                   m[9] = 0.0f;                   m[10] = -2.0f / fn;          m[11] = 0.0f;
    m[12] = -(right + left) / rl;  m[13] = -(top + bottom) / tb;  m[14] = -(far + near) / fn;  m[15] = 1.0f;
}

void invertOrtho(float* m, float* inv) {
    inv[0] = 1.0f / m[0];      inv[1] = 0.0f;             inv[2] = 0.0f;              inv[3] = 0.0f;                
    inv[4] = 0.0f;             inv[5] = 1.0f / m[5];      inv[6] = 0.0f;              inv[7] = 0.0f;                
    inv[8] = 0.0f;             inv[9] = 0.0f;             inv[10] = 1.0f / m[10];     inv[11] = 0.0f;                
    inv[12] = -m[12] * inv[0]; inv[13] = -m[13] * inv[5]; inv[14] = -m[14] * inv[10]; inv[15] = 1.0f;                
}