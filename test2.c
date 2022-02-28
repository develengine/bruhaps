#include "bag_engine.h"
#include "bag_keys.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define length(x) (sizeof(x)/sizeof(*x))

static int running = 1;
int windowWidth, windowHeight;


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


static void printContextInfo(void)
{

    printf("Adaptive vsync: %d\n", bagE_isAdaptiveVsyncAvailable());

    const char *vendorString = (const char*)glGetString(GL_VENDOR);
    const char *rendererString = (const char*)glGetString(GL_RENDERER);
    const char *versionString = (const char*)glGetString(GL_VERSION);
    const char *shadingLanguageVersionString = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("Vendor: %s\nRenderer: %s\nVersion: %s\nShading Language version: %s\n",
        vendorString, rendererString, versionString, shadingLanguageVersionString);
}


/* WOAH */
static char *readFile(const char *name)
{
    FILE *file = NULL;
    char *contents = NULL;

    file = fopen(name, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    if (fseek(file, 0, SEEK_END)) {
        fprintf(stderr, "Failed to seek in file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    long size = ftell(file);
    if (size == -1L) {
        fprintf(stderr, "Failed to tell in file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    rewind(file);

    contents = malloc(size + 1); /* null byte */
    if (!contents) {
        fprintf(stderr, "Failed to allocate memory!\n");
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    if (fread(contents, 1, size, file) != (size_t)size) {
        fprintf(stderr, "Failed to read file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    if (fclose(file)) {
        fprintf(stderr, "Failed to close file \"%s\"!\n", name);
    }

    contents[size] = 0;
    return contents;

error_exit:
    if (file) {
        fclose(file);
    }
    free(contents);
    return NULL;
}


static int loadShader(const char *path, GLenum type)
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


static int createProgram(const char *vertexPath, const char *fragmentPath)
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


static float cubeVertices[] = {
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

static unsigned cubeIndices[] = {
    // front face
    0, 1, 2,
    2, 3, 0,
    // TODO
};


typedef struct {
    float data[16];
} Matrix;


static inline Matrix matrixIdentity(void)
{
    Matrix res = {{
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f,
    }};
    return res;
}


static inline Matrix matrixScale(float x, float y, float z)
{
    Matrix res = {{
         x,    0.0f, 0.0f, 0.0f,
         0.0f, y,    0.0f, 0.0f,
         0.0f, 0.0f, z,    0.0f,
         0.0f, 0.0f, 0.0f, 1.0f,
    }};
    return res;
}


static inline Matrix matrixTranslation(float x, float y, float z)
{
    Matrix res = {{
         1.0f, 0.0f, 0.0f, x,
         0.0f, 1.0f, 0.0f, y,
         0.0f, 0.0f, 1.0f, z,
         0.0f, 0.0f, 0.0f, 1.0f,
    }};
    return res;
}


static inline Matrix matrixRotationX(float angle)
{
    Matrix res = {{
         1.0f, 0.0f,        0.0f,        0.0f,
         0.0f, cosf(angle),-sinf(angle), 0.0f,
         0.0f, sinf(angle), cosf(angle), 0.0f,
         0.0f, 0.0f,        0.0f,        1.0f,
    }};
    return res;
}


static inline Matrix matrixRotationY(float angle)
{
    Matrix res = {{
         cosf(angle), 0.0f, sinf(angle), 0.0f,
         0.0f,        1.0f, 0.0f,        0.0f,
        -sinf(angle), 0.0f, cosf(angle), 0.0f,
         0.0f,        0.0f, 0.0f,        1.0f,
    }};
    return res;
}


static inline Matrix matrixMultiply(const Matrix *mat1, const Matrix *mat2)
{
    const float *m1 = mat1->data;
    const float *m2 = mat2->data;

    Matrix res;

    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            res.data[y * 4 + x] = m1[y * 4 + 0] * m2[0 * 4 + x]
                                + m1[y * 4 + 1] * m2[1 * 4 + x]
                                + m1[y * 4 + 2] * m2[2 * 4 + x]
                                + m1[y * 4 + 3] * m2[3 * 4 + x];
        }
    }

    return res;
}


static void printMatrix(const Matrix *mat)
{
    for (int y = 0; y < 4; ++y) {
        printf("[ ");
        for (int x = 0; x < 4; ++x) {
            printf("%f ", mat->data[y * 4 + x]);
        }
        printf("]\n");
    }
}


int bagE_main(int argc, char *argv[])
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0);

    printContextInfo();

    bagE_setWindowTitle("BRUHAPS");
    bagE_getWindowSize(&windowWidth, &windowHeight);
    bagE_setSwapInterval(1);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    Matrix mat = matrixScale(3.0f, 1.0f, 2.0f);
    Matrix tmp = matrixTranslation(0.5f, 0.6f, 0.3f);
    mat = matrixMultiply(&mat, &tmp);
    printMatrix(&mat);

    int program = createProgram("shaders/proper_vert.glsl", "shaders/proper_frag.glsl");

    unsigned vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, sizeof(cubeVertices), cubeVertices, 0);

    unsigned ebo;
    glCreateBuffers(1, &ebo);
    glNamedBufferStorage(ebo, sizeof(cubeIndices), cubeIndices, 0);

    unsigned vao;
    glCreateVertexArrays(1, &vao);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 3);
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glVertexArrayElementBuffer(vao, ebo);
    

    glBindVertexArray(vao);
    glUseProgram(program);

    double dt = 0;

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

        glClearColor(
                0.1f,
                0.2f,
                0.3f,
                1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT);

        glProgramUniform3f(program, 0, 0.25f, 0.0f, 0.0f);
        glProgramUniform3f(program, 1, 0.5f, 0.5f, 0.5f);
        glProgramUniform4f(program, 2,
                0.1f,
                (sin(dt) + 1.0) * 0.5,
                0.5f,
                1.0f
        );

        glDrawElements(GL_TRIANGLES, length(cubeIndices), GL_UNSIGNED_INT, 0);

        bagE_swapBuffers();

        dt += 0.1;
    }

  
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(program);

    return 0;
}


int bagE_eventHandler(bagE_Event *event)
{
    switch (event->type)
    {
        case bagE_EventWindowClose:
            running = 0;
            return 1;

        case bagE_EventWindowResize: {
            bagE_WindowResize *wr = &(event->data.windowResize);
            windowWidth = wr->width;
            windowHeight = wr->height;
            glViewport(0, 0, wr->width, wr->height);
        } break;

        default: break;
    }

    return 0;
}
