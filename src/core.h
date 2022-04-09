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
AnimatedObject createAnimatedObject(Animated animated);

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
