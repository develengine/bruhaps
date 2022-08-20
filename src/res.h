#ifndef RES_H
#define RES_H

#include "linalg.h"

typedef struct
{
    float positions[3];
    float textures[2];
    float normals[3];
} Vertex;


typedef struct
{
    int vertexCount;
    int indexCount;
    Vertex *vertices;
    unsigned *indices;
} Model;


typedef struct
{
    unsigned ids[4];
    float weights[4];
} VertexWeight;


typedef struct
{
    float position[4];
    Quaternion rotation;
} JointTransform;


typedef struct
{
    int boneCount;

    Matrix *ibms;

    unsigned *frameCounts;
    unsigned *frameOffsets;
    float *timeStamps;
    JointTransform *transforms;

    unsigned *childCounts;
    unsigned *childOffsets;
    unsigned *hierarchy;
} Armature;


typedef struct
{
    Model model;
    Armature armature;
    VertexWeight *vertexWeights;
} Animated;


Model modelLoad(const char *path);
void modelPrint(const Model *model);
void modelFree(Model model);

Animated animatedLoad(const char *path);
void animatedFree(Animated animated);

void armatureFree(Armature armature);

#endif
