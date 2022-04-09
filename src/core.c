#include "core.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


void GLAPIENTRY openglCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
) {
    int error = type == GL_DEBUG_TYPE_ERROR;

    printf("[%s] type: %d, severity: %d\n%s\n",
            error ? "\033[1;31mERROR\033[0m" : "\033[1mINFO\033[0m",
            type, severity, message
    );
}


void printContextInfo(void)
{
    printf("Adaptive vsync: %d\n", bagE_isAdaptiveVsyncAvailable());

    const char *vendorString = (const char*)glGetString(GL_VENDOR);
    const char *rendererString = (const char*)glGetString(GL_RENDERER);
    const char *versionString = (const char*)glGetString(GL_VERSION);
    const char *shadingLanguageVersionString = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("Vendor: %s\nRenderer: %s\nVersion: %s\nShading Language version: %s\n",
        vendorString, rendererString, versionString, shadingLanguageVersionString);
}


static const float cubeVertices[] = {
    // front face
    -1.0f, 1.0f, 1.0f,  // 0      3
    -1.0f,-1.0f, 1.0f,  // 
     1.0f,-1.0f, 1.0f,  // 
     1.0f, 1.0f, 1.0f,  // 1      2
    // back face
    -1.0f, 1.0f,-1.0f,  // 4      7
    -1.0f,-1.0f,-1.0f,  // 
     1.0f,-1.0f,-1.0f,  // 
     1.0f, 1.0f,-1.0f,  // 5      6
};

static const unsigned cubeIndices[] = {
    // front face
    0, 1, 2,
    2, 3, 0,
    // right face
    3, 2, 6,
    6, 7, 3,
    // left face
    4, 5, 1,
    1, 0, 4,
    // back face
    7, 6, 5,
    5, 4, 7,
    // top face
    3, 7, 4,
    4, 0, 3,
    // bottom face
    1, 5, 6,
    6, 2, 1,
};

static const unsigned boxIndices[] = {
    2, 1, 0,
    0, 3, 2,
    6, 2, 3,
    3, 7, 6,
    1, 5, 4,
    4, 0, 1,
    5, 6, 7,
    7, 4, 5,
    4, 7, 3,
    3, 0, 4,
    6, 5, 1,
    1, 2, 6,
};


ModelObject createCubeModelObject(void)
{
    ModelObject object;

    glCreateBuffers(1, &object.vbo);
    glNamedBufferStorage(object.vbo, sizeof(cubeVertices), cubeVertices, 0);

    glCreateBuffers(1, &object.ebo);
    glNamedBufferStorage(object.ebo, sizeof(cubeIndices), cubeIndices, 0);

    glCreateVertexArrays(1, &object.vao);
    glVertexArrayVertexBuffer(object.vao, 0, object.vbo, 0, sizeof(float) * 3);
    glEnableVertexArrayAttrib(object.vao, 0);
    glVertexArrayAttribFormat(object.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(object.vao, 0, 0);

    glVertexArrayElementBuffer(object.vao, object.ebo);

    return object;
}


ModelObject createBoxModelObject(void)
{
    ModelObject object;

    glCreateBuffers(1, &object.vbo);
    glNamedBufferStorage(object.vbo, sizeof(cubeVertices), cubeVertices, 0);

    glCreateBuffers(1, &object.ebo);
    glNamedBufferStorage(object.ebo, sizeof(boxIndices), boxIndices, 0);

    glCreateVertexArrays(1, &object.vao);
    glVertexArrayVertexBuffer(object.vao, 0, object.vbo, 0, sizeof(float) * 3);
    glEnableVertexArrayAttrib(object.vao, 0);
    glVertexArrayAttribFormat(object.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(object.vao, 0, 0);

    glVertexArrayElementBuffer(object.vao, object.ebo);

    return object;
}


int loadShader(const char *path, GLenum type)
{
    char *source = readFile(path);
    if (!source)
        exit(1);

    int shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char * const*)&source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "[\033[1;31mERROR\033[0m] Failed to compile shader '%s'!\n"
                        "%s", path, infoLog);
        exit(1);
    }

    free(source);
    return shader;
}


int createProgram(const char *vertexPath, const char *fragmentPath)
{
    int vertexShader = loadShader(vertexPath, GL_VERTEX_SHADER);
    int fragmentShader = loadShader(fragmentPath, GL_FRAGMENT_SHADER);
    int program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "[\033[1;31mERROR\033[0m] Failed to link shader program!\n"
                        "%s", infoLog);
        exit(1);
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}


uint8_t *loadImage(const char *path, int *width, int *height, int *channels, bool flip)
{
    stbi_set_flip_vertically_on_load(flip);

    uint8_t *image = stbi_load(path, width, height, channels, STBI_rgb_alpha);
    if (!image) {
        fprintf(stderr, "Failed to load image \"%s\"\n", path);
        exit(1);
    }

    return image;
}


unsigned createTexture(const char *path)
{
    int width, height, channelCount;
    uint8_t *image = loadImage(path, &width, &height, &channelCount, true);

    unsigned texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTextureStorage2D(texture, log2(width), GL_RGBA8, width, height);
    glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glGenerateTextureMipmap(texture);

    free(image);

    return texture;
}


ModelObject createModelObject(Model model)
{
    ModelObject object;

    glCreateBuffers(1, &object.vbo);
    glNamedBufferStorage(object.vbo, model.vertexCount * sizeof(Vertex), model.vertices, 0);

    glCreateBuffers(1, &object.ebo);
    glNamedBufferStorage(object.ebo, model.indexCount * sizeof(unsigned), model.indices, 0);

    glCreateVertexArrays(1, &object.vao);
    glVertexArrayVertexBuffer(object.vao, 0, object.vbo, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(object.vao, 0);
    glEnableVertexArrayAttrib(object.vao, 1);
    glEnableVertexArrayAttrib(object.vao, 2);

    glVertexArrayAttribFormat(object.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(object.vao, 1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
    glVertexArrayAttribFormat(object.vao, 2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5);

    glVertexArrayAttribBinding(object.vao, 0, 0);
    glVertexArrayAttribBinding(object.vao, 1, 0);
    glVertexArrayAttribBinding(object.vao, 2, 0);

    glVertexArrayElementBuffer(object.vao, object.ebo);

    return object;
}


AnimatedObject createAnimatedObject(Animated animated)
{
    AnimatedObject object;

    glCreateBuffers(1, &object.model.vbo);
    glNamedBufferStorage(
            object.model.vbo,
            animated.model.vertexCount * sizeof(Vertex),
            animated.model.vertices,
            0
    );

    glCreateBuffers(1, &object.weights);
    glNamedBufferStorage(
            object.weights,
            animated.model.vertexCount * sizeof(VertexWeight),
            animated.vertexWeights,
            0
    );

    glCreateBuffers(1, &object.model.ebo);
    glNamedBufferStorage(
            object.model.ebo,
            animated.model.indexCount * sizeof(unsigned),
            animated.model.indices,
            0
    );

    glCreateVertexArrays(1, &object.model.vao);
    glVertexArrayVertexBuffer(object.model.vao, 0, object.model.vbo, 0, sizeof(Vertex));
    glVertexArrayVertexBuffer(object.model.vao, 1, object.weights, 0, sizeof(VertexWeight));

    glEnableVertexArrayAttrib(object.model.vao, 0);
    glEnableVertexArrayAttrib(object.model.vao, 1);
    glEnableVertexArrayAttrib(object.model.vao, 2);
    glEnableVertexArrayAttrib(object.model.vao, 3);
    glEnableVertexArrayAttrib(object.model.vao, 4);

    glVertexArrayAttribFormat(object.model.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(object.model.vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
    glVertexArrayAttribFormat(object.model.vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
    glVertexArrayAttribIFormat(object.model.vao, 3, 4, GL_UNSIGNED_INT, 0);
    glVertexArrayAttribFormat(object.model.vao, 4, 4, GL_FLOAT, GL_FALSE, sizeof(unsigned) * 4);

    glVertexArrayAttribBinding(object.model.vao, 0, 0);
    glVertexArrayAttribBinding(object.model.vao, 1, 0);
    glVertexArrayAttribBinding(object.model.vao, 2, 0);
    glVertexArrayAttribBinding(object.model.vao, 3, 1);
    glVertexArrayAttribBinding(object.model.vao, 4, 1);

    glVertexArrayElementBuffer(object.model.vao, object.model.ebo);

    return object;
}

unsigned createCubeTexture(
        const char *pxPath,
        const char *nxPath,
        const char *pyPath,
        const char *nyPath,
        const char *pzPath,
        const char *nzPath
) {
    const char *paths[] = { pxPath, nxPath, pyPath, nyPath, pzPath, nzPath };

    int width, height, channels;
    uint8_t *images[6];

    images[0] = loadImage(paths[0], &width, &height, &channels, false);

    assert(width == height);

    for (int i = 1; i < 6; ++i) {
        int w, h, c;
        images[i] = loadImage(paths[i], &w, &h, &c, false);

        assert(w == width);
        assert(h == height);
        assert(c == channels);
    }

    unsigned texture;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);
    glTextureStorage2D(texture, log2(width) + 1, GL_RGBA8, width, height);

    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    for (int i = 0; i < 6; ++i) {
        glTextureSubImage3D(
                texture,
                0,
                0, 0, i,
                width, height, 1,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                images[i]
        );

        free(images[i]);
    }

    glGenerateTextureMipmap(texture);

    return texture;
}



