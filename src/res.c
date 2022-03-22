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
    safe_read(&animated.boneCount, sizeof(int), 1, file);

    animated.model.vertices = malloc(sizeof(Vertex) * animated.model.vertexCount);
    animated.model.indices = malloc(sizeof(unsigned) * animated.model.indexCount);
    malloc_check(animated.model.vertices);
    malloc_check(animated.model.indices);
    safe_read(animated.model.vertices, sizeof(Vertex), animated.model.vertexCount, file);
    safe_read(animated.model.indices, sizeof(unsigned), animated.model.indexCount, file);

    animated.vertexWeights = malloc(sizeof(VertexWeight) * animated.model.vertexCount);
    animated.ibms = malloc(sizeof(Matrix) * animated.boneCount);
    malloc_check(animated.vertexWeights);
    malloc_check(animated.ibms);
    safe_read(animated.vertexWeights, sizeof(VertexWeight), animated.model.vertexCount, file);
    safe_read(animated.ibms, sizeof(Matrix), animated.boneCount, file);

    animated.frameCounts = malloc(sizeof(unsigned) * animated.boneCount);
    malloc_check(animated.frameCounts);
    safe_read(animated.frameCounts, sizeof(unsigned), animated.boneCount, file);

    int frameCount = 0;
    for (int i = 0; i < animated.boneCount; ++i)
        frameCount += animated.frameCounts[i];

    animated.timeStamps = malloc(sizeof(float) * frameCount);
    animated.transforms = malloc(sizeof(JointTransform) * frameCount);
    malloc_check(animated.timeStamps);
    malloc_check(animated.transforms);
    safe_read(animated.timeStamps, sizeof(float), frameCount, file);
    safe_read(animated.transforms, sizeof(JointTransform), frameCount, file);

    animated.childCounts = malloc(sizeof(unsigned) * animated.boneCount);
    malloc_check(animated.childCounts);
    safe_read(animated.childCounts, sizeof(unsigned), animated.boneCount, file);

    int childrenCount = 0;
    for (int i = 0; i < animated.boneCount; ++i)
        childrenCount += animated.childCounts[i];

    animated.hierarchy = malloc(sizeof(unsigned) * childrenCount);
    malloc_check(animated.hierarchy);
    safe_read(animated.childCounts, sizeof(unsigned), childrenCount, file);

    eof_check(file);

    fclose(file);
    return animated;
}


void animatedFree(Animated animated)
{
    modelFree(animated.model);
    free(animated.vertexWeights);
    free(animated.ibms);
    free(animated.frameCounts);
    free(animated.timeStamps);
    free(animated.transforms);
    free(animated.childCounts);
    free(animated.hierarchy);
}


