#ifndef CORE_H
#define CORE_H

#include "bag_engine.h"
#include "res.h"

#include <stdbool.h>

typedef struct
{
    unsigned vao;
    unsigned vbo;
    unsigned ebo;
    unsigned indexCount;
} ModelObject;

static inline void freeModelObject(ModelObject object)
{
    glDeleteVertexArrays(1, &object.vao);
    glDeleteBuffers(1, &object.vbo);
    glDeleteBuffers(1, &object.ebo);
}


typedef struct
{
    ModelObject model;
    unsigned weights;
} AnimatedObject;

static inline void freeAnimatedObject(AnimatedObject object)
{
    freeModelObject(object.model);
    glDeleteBuffers(1, &object.weights);
}


void GLAPIENTRY openglCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
);

void printContextInfo(void);

int loadShader(const char *path, GLenum type);
int createProgram(const char *vertexPath, const char *fragmentPath);
uint8_t *loadImage(const char *path, int *width, int *height, int *channels, bool flip);
unsigned createTexture(const char *path);
ModelObject createModelObject(Model model);
ModelObject loadModelObject(const char *path);
AnimatedObject createAnimatedObject(Animated animated);
unsigned createBufferObject(size_t size, void *data, unsigned flags);

unsigned createCubeTexture(
        const char *pxPath,
        const char *nxPath,
        const char *pyPath,
        const char *nyPath,
        const char *pzPath,
        const char *nzPath
);

ModelObject createCubeModelObject(void);
ModelObject createBoxModelObject(void);


#define BOX_INDEX_COUNT 36

#endif
