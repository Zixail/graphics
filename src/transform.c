#include <math.h>

typedef struct Transform {
    float x, y, z;
    float rotationZ;
    float sx, sy, sz;
    float model[16];
} Transform;

void transformInit(Transform* t){
    t->x = t->y = t->z = 0.0f;
    t->rotationZ = 0.0f;
    t->sx = t-> sy = t->sz = 1.0f;
    float *m = t->model;

    m[0] = 1.0f;  m[1] = 0.0f;   m[2] = 0.0f;   m[3] = 0.0f;
    m[4] = 0.0f;  m[5] = 1.0f;   m[6] = 0.0f;   m[7] = 0.0f;
    m[8] = 0.0f;  m[9] = 0.0f;   m[10] = 1.0f;  m[11] = 0.0f;
    m[12] = 0.0f; m[13] = 0.0f;  m[14] = 0.0f;  m[15] = 1.0f;
}

void transformUpdateModel(Transform* t){
    float x = t->x;
    float y = t->y;
    float z = t->z;

    float sx = t->sx;
    float sy = t->sy;
    //float sz = t->sz;

    float c = cosf(t->rotationZ);
    float s = sinf(t->rotationZ);

    float *m = t->model;

    m[0] = c * sx;   m[1] = s * sx;   m[2] = 0.0f;   m[3] = 0.0f;
    m[4] = -s * sy;  m[5] = c * sy;   m[6] = 0.0f;   m[7] = 0.0f;
    m[8] = 0.0f;     m[9] = 0.0f;     m[10] = 1.0f;  m[11] = 0.0f;
    m[12] = x;       m[13] = y;       m[14] = z;     m[15] = 1.0f;
}

void makeOrtho(float left, float right, float bottom, float top, float near, float far, float* m){
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;

    m[0] = 2.0f / rl;              m[1] = 0.0f;                   m[2] = 0.0f;                 m[3] = 0.0f;                     
    m[4] = 0.0f;                   m[5] = 2.0f / tb;              m[6] = 0.0f;                 m[7] = 0.0f;
    m[8] = 0.0f;                   m[9] = 0.0f;                   m[10] = -2.0f / fn;          m[11] = 0.0f;
    m[12] = -(right + left) / rl;  m[13] = -(top + bottom) / tb;  m[14] = -(far + near) / fn;  m[15] = 1.0f;
}