#ifndef TRANSFORM_H
#define TRANSFORM_H

typedef struct Projection {
    float mat[16];
} Projection;

typedef struct Transform {
    float x, y;
    float rotationZ;
    float sx, sy;
    float mat[16];
} Transform;

void initTransform(Transform* t);
void updateTransform(Transform* t);

void makeOrtho(float left, float right, 
                float bottom, float top, 
                float near, float far, Projection* projection);

void invertOrtho(Projection* proj, Projection* i);
Projection* getProjection();

#endif