#include "res.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

Model modelLoad(const char *path)
{
    Model model;
    FILE *file = fopen(path, "rb");
    file_check(file, path);

    safe_read(&model.vertexCount, sizeof(int), 1, file);
    safe_read(&model.indexCount, sizeof(int), 1, file);

    model.vertices = malloc(sizeof(Vertex) * model.vertexCount);
    malloc_check(model.vertices);

    model.indices = malloc(sizeof(unsigned) * model.indexCount);
    malloc_check(model.indices);

    safe_read(model.vertices, sizeof(Vertex), model.vertexCount, file);
    safe_read(model.indices, sizeof(unsigned), model.indexCount, file);

    eof_check(file);

    fclose(file);
    return model;
}


void modelPrint(const Model *model)
{
    printf("vertexCount: %d\n", model->vertexCount);
    printf("indexCount:  %d\n", model->indexCount);
    for (int i = 0; i < model->vertexCount; ++i) {
        printf("[%f, %f, %f], [%f, %f], [%f, %f, %f]\n",
                model->vertices[i].positions[0],
                model->vertices[i].positions[1],
                model->vertices[i].positions[2],

                model->vertices[i].textures[0],
                model->vertices[i].textures[1],

                model->vertices[i].normals[0],
                model->vertices[i].normals[1],
                model->vertices[i].normals[2]
        );
    }
    for (int i = 0; i < model->indexCount; i += 3) {
        printf("[%u, %u, %u]\n",
                model->indices[i],
                model->indices[i + 1],
                model->indices[i + 2]
        );
    }
}


void modelFree(Model model)
{
    free(model.vertices);
    free(model.indices);
}


Animated animatedLoad(const char *path)
{
    Animated animated;

    FILE *file = fopen(path, "rb");
    file_check(file, path);

    safe_read(&animated.model.vertexCount, sizeof(int), 1, file);
    safe_read(&animated.model.indexCount, sizeof(int), 1, file);
    safe_read(&animated.armature.boneCount, sizeof(int), 1, file);

    animated.model.vertices = malloc(sizeof(Vertex) * animated.model.vertexCount);
    animated.model.indices = malloc(sizeof(unsigned) * animated.model.indexCount);
    malloc_check(animated.model.vertices);
    malloc_check(animated.model.indices);
    safe_read(animated.model.vertices, sizeof(Vertex), animated.model.vertexCount, file);
    safe_read(animated.model.indices, sizeof(unsigned), animated.model.indexCount, file);

    animated.vertexWeights = malloc(sizeof(VertexWeight) * animated.model.vertexCount);
    animated.armature.ibms = malloc(sizeof(Matrix) * animated.armature.boneCount);
    malloc_check(animated.vertexWeights);
    malloc_check(animated.armature.ibms);
    safe_read(animated.vertexWeights, sizeof(VertexWeight), animated.model.vertexCount, file);
    safe_read(animated.armature.ibms, sizeof(Matrix), animated.armature.boneCount, file);

    animated.armature.frameCounts = malloc(sizeof(unsigned) * animated.armature.boneCount);
    animated.armature.frameOffsets = malloc(sizeof(unsigned) * animated.armature.boneCount);
    malloc_check(animated.armature.frameCounts);
    malloc_check(animated.armature.frameOffsets);
    safe_read(animated.armature.frameCounts, sizeof(unsigned), animated.armature.boneCount, file);

    int frameCount = 0;
    for (int i = 0; i < animated.armature.boneCount; ++i) {
        animated.armature.frameOffsets[i] = frameCount;
        frameCount += animated.armature.frameCounts[i];
    }

    animated.armature.timeStamps = malloc(sizeof(float) * frameCount);
    animated.armature.transforms = malloc(sizeof(JointTransform) * frameCount);
    malloc_check(animated.armature.timeStamps);
    malloc_check(animated.armature.transforms);
    safe_read(animated.armature.timeStamps, sizeof(float), frameCount, file);
    safe_read(animated.armature.transforms, sizeof(JointTransform), frameCount, file);

    animated.armature.childCounts = malloc(sizeof(unsigned) * animated.armature.boneCount);
    animated.armature.childOffsets = malloc(sizeof(unsigned) * animated.armature.boneCount);
    malloc_check(animated.armature.childCounts);
    malloc_check(animated.armature.childOffsets);
    safe_read(animated.armature.childCounts, sizeof(unsigned), animated.armature.boneCount, file);

    int childrenCount = 0;
    for (int i = 0; i < animated.armature.boneCount; ++i) {
        animated.armature.childOffsets[i] = childrenCount;
        childrenCount += animated.armature.childCounts[i];
    }

    animated.armature.hierarchy = malloc(sizeof(unsigned) * childrenCount);
    malloc_check(animated.armature.hierarchy);
    safe_read(animated.armature.hierarchy, sizeof(unsigned), childrenCount, file);

    eof_check(file);

    fclose(file);
    return animated;
}


void animatedFree(Animated animated)
{
    modelFree(animated.model);
    free(animated.vertexWeights);
    free(animated.armature.ibms);
    free(animated.armature.frameCounts);
    free(animated.armature.timeStamps);
    free(animated.armature.transforms);
    free(animated.armature.childCounts);
    free(animated.armature.hierarchy);
}


