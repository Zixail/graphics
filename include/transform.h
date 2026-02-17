#ifndef TRANSFORM_H
#define TRANSFORM_H

typedef struct Transform {
    float x, y, z;
    float rotationZ;
    float sx, sy, sz;
    float model[16];
} Transform;

void transformInit(Transform* t);
void transformUpdateModel(Transform* t);

void makeOrtho(float left, float right, 
                float bottom, float top, 
                float near, float far, float* m);

#endif