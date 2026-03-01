#ifndef TRANSFORM_H
#define TRANSFORM_H

typedef struct Projection {
    float mat[16];
} Projection;

typedef struct Transform {
    float x, y;
    float rotationZ;
    float sx, sy;
    float model[16];
} Transform;

void transformInit(Transform* t);
void transformUpdateModel(Transform* t);

void makeOrtho(float left, float right, 
                float bottom, float top, 
                float near, float far, float* m);

void invertOrtho(Projection* proj, Projection* i);

#endif