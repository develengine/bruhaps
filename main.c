#include "bag_engine.h"
#include "bag_keys.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define length(x) (sizeof(x)/sizeof(*x))


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
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         x,    y,    z,    1.0f,
    }};
    return res;
}


static inline Matrix matrixRotationX(float angle)
{
    Matrix res = {{
         1.0f, 0.0f,        0.0f,        0.0f,
         0.0f, cosf(angle), sinf(angle), 0.0f,
         0.0f,-sinf(angle), cosf(angle), 0.0f,
         0.0f, 0.0f,        0.0f,        1.0f,
    }};
    return res;
}


static inline Matrix matrixRotationY(float angle)
{
    Matrix res = {{
         cosf(angle), 0.0f,-sinf(angle), 0.0f,
         0.0f,        1.0f, 0.0f,        0.0f,
         sinf(angle), 0.0f, cosf(angle), 0.0f,
         0.0f,        0.0f, 0.0f,        1.0f,
    }};
    return res;
}


static inline Matrix matrixRotationZ(float angle)
{
    Matrix res = {{
         cosf(angle), sinf(angle), 0.0f, 0.0f,
        -sinf(angle), cosf(angle), 0.0f, 0.0f,
         0.0f,        0.0f,        1.0f, 0.0f,
         0.0f,        0.0f,        0.0f, 1.0f,
    }};
    return res;
}


#if 1
static inline Matrix matrixProjection(
        float fov,
        float width,
        float height,
        float np,
        float fp
) {
    float ar = height / width;
    float xs = (float)((1.0 / tan((fov / 2.0) * M_PI / 180)) * ar);
    float ys = xs / ar;
    float flen = fp - np;

    Matrix res = {{
         xs,   0.0f, 0.0f,                   0.0f,
         0.0f, ys,   0.0f,                   0.0f,
         0.0f, 0.0f,-((fp + np) / flen),    -1.0f,
         0.0f, 0.0f,-((2 * np * fp) / flen), 0.0f, // NOTE if 1.0f removes far plane clipping ???
    }};

    return res;
}
#else
static inline Matrix matrixProjection(
        float fov,
        float width,
        float height,
        float np,
        float fp
) {
    float top = np * tanf((M_PI * fov) / 360.0f);
    float bottom = -top;
    float right = top * (width / height);
    float left = -right;

    Matrix res = {{
         (2 * np) / (right - left), 0.0f, 0.0f, 0.0f,
         0.0f, (2 * np) / (top - bottom), 0.0f, 0.0f,
         (right + left) / (right - left), (top + bottom) / (top - bottom),-((fp + np) / (fp - np)),-1.0f,
         0.0f, 0.0f, -((2 * fp * np)/(fp - np)), 0.0f,
    }};

    return res;
}
#endif

static inline Matrix matrixMultiply(const Matrix *mat1, const Matrix *mat2)
{
    const float *m1 = mat1->data;
    const float *m2 = mat2->data;

    Matrix res;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float value = 0;

            for (int k = 0; k < 4; k++)
                value += m2[j * 4 + k] * m1[k * 4 + i];

            res.data[j * 4 + i] = value;
        }
    }

    return res;
}



void printMatrix(const Matrix *mat)
{
    printf("\n");
    for (int y = 0; y < 4; ++y) {
        printf("[ ");
        for (int x = 0; x < 4; ++x) {
            printf("%f ", mat->data[y * 4 + x]);
        }
        printf("]\n");
    }
}


static const float MOUSE_SENSITIVITY = 0.005f;


static int running = 1;
static int windowWidth, windowHeight;
static int windowChanged = 0;

static int playerInput = 0;
static bool fullscreen = false;

static bool leftDown    = false;
static bool rightDown   = false;
static bool forthDown   = false;
static bool backDown    = false;
static bool ascendDown  = false;
static bool descendDown = false;

static bool altDown = false;
static bool fDown   = false;

static float motionYaw   = 0.0f;
static float motionPitch = 0.0f;


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


static Model modelLoad(const char *path)
{
    Model model;
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file \"%s\"!\n", path);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        exit(-1);
    }

    // TODO FIXME read failure handling
    fread(&model.vertexCount, sizeof(int), 1, file);
    fread(&model.indexCount, sizeof(int), 1, file);

    model.vertices = malloc(sizeof(Vertex) * model.vertexCount);
    if (!model.vertices) {
        fprintf(stderr, "malloc fail: %s, %d\n", __FILE__, __LINE__);
        exit(-1);
    }

    model.indices = malloc(sizeof(unsigned) * model.indexCount);
    if (!model.indices) {
        fprintf(stderr, "malloc fail: %s, %d\n", __FILE__, __LINE__);
        exit(-1);
    }

    // TODO FIXME read failure handling
    fread(model.vertices, sizeof(Vertex), model.vertexCount, file);
    fread(model.indices, sizeof(unsigned), model.indexCount, file);

    fclose(file);
    return model;
}


static void modelPrint(const Model *model)
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


static void modelFree(Model *model)
{
    free(model->vertices);
    free(model->indices);
}


int bagE_main(int argc, char *argv[])
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0);

    printContextInfo();

    bagE_setWindowTitle("BRUHAPS");
    bagE_getWindowSize(&windowWidth, &windowHeight);
    bagE_setSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_MULTISAMPLE);
    // glEnable(GL_MULTISAMPLE);

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




    Model model = modelLoad("energy.model");
    // modelPrint(&model);

    printf("sizeof(Vertex): %lu\n", sizeof(Vertex));


    int modelProgram = createProgram("shaders/3d_vertex.glsl", "shaders/3d_fragment.glsl");

    unsigned modelVbo;
    glCreateBuffers(1, &modelVbo);
    glNamedBufferStorage(modelVbo, model.vertexCount * sizeof(Vertex), model.vertices, 0);

    unsigned modelEbo;
    glCreateBuffers(1, &modelEbo);
    glNamedBufferStorage(modelEbo, model.indexCount * sizeof(unsigned), model.indices, 0);

    unsigned modelVao;
    glCreateVertexArrays(1, &modelVao);
    glVertexArrayVertexBuffer(modelVao, 0, modelVbo, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(modelVao, 0);
    glEnableVertexArrayAttrib(modelVao, 1);
    glEnableVertexArrayAttrib(modelVao, 2);

    glVertexArrayAttribFormat(modelVao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(modelVao, 1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
    glVertexArrayAttribFormat(modelVao, 2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5);

    glVertexArrayAttribBinding(modelVao, 0, 0);
    glVertexArrayAttribBinding(modelVao, 1, 0);
    glVertexArrayAttribBinding(modelVao, 2, 0);

    glVertexArrayElementBuffer(modelVao, modelEbo);

    double t = 0;

    float camX = 0.0f, camY = 0.0f, camZ = 0.0f;
    float camPitch = 0.0f, camYaw = 0.0f;

    float objX, objY, objZ;
    float objScale = 1.0f;

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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camPitch += motionPitch * MOUSE_SENSITIVITY;
        camYaw   += motionYaw   * MOUSE_SENSITIVITY;
        motionPitch = 0.0f;
        motionYaw   = 0.0f;

        if (playerInput) {
            if (leftDown) {
                camX -= 0.1f * cosf(camYaw);
                camZ -= 0.1f * sinf(camYaw);
            }
            if (rightDown) {
                camX += 0.1f * cosf(camYaw);
                camZ += 0.1f * sinf(camYaw);
            }
            if (forthDown) {
                camX += 0.1f * sinf(camYaw);
                camZ -= 0.1f * cosf(camYaw);
            }
            if (backDown) {
                camX -= 0.1f * sinf(camYaw);
                camZ += 0.1f * cosf(camYaw);
            }
            if (ascendDown)
                camY += 0.1f;
            if (descendDown)
                camY -= 0.1f;
        }

        objX = 0.0f;
        objY = 0.0f;
        objZ = -10.0f;

        Matrix mul;

        /* model */
        Matrix modelm = matrixScale(objScale, objScale, objScale);

        // mul = matrixRotationY(t / 4);
        // modelm = matrixMultiply(&mul, &modelm);

        mul = matrixTranslation(objX, objY, objZ);
        modelm = matrixMultiply(&mul, &modelm);

        /* view */
        Matrix view = matrixTranslation(-camX,-camY,-camZ);

        mul = matrixRotationY(camYaw);
        view = matrixMultiply(&mul, &view);

        mul = matrixRotationX(camPitch);
        view = matrixMultiply(&mul, &view);

        /* projection */
        Matrix proj = matrixProjection(90.0f, windowWidth, windowHeight, 0.1f, 100.0f);

        /* mvp */
        Matrix mvp = matrixMultiply(&view, &modelm);
        mvp = matrixMultiply(&proj, &mvp);

        /* vp */
        Matrix vp = matrixMultiply(&proj, &view);


        glBindVertexArray(modelVao);
        glUseProgram(modelProgram);

        glProgramUniformMatrix4fv(modelProgram, 0, 1, GL_FALSE, vp.data);
        glProgramUniformMatrix4fv(modelProgram, 1, 1, GL_FALSE, modelm.data);
        glProgramUniform3f(modelProgram, 2, camX, camY, camZ);
        glProgramUniform3f(modelProgram, 3,
                0.1f,
                // (sin(t) + 1.0) * 0.5,
                1.0f,
                0.5f
        );

        glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, 0);


        /*
        glBindVertexArray(vao);
        glUseProgram(program);

        glProgramUniformMatrix4fv(program, 0, 1, GL_FALSE, mvp.data);
        glProgramUniform4f(program, 1,
                0.1f,
                (sin(t) + 1.0) * 0.5,
                0.5f,
                1.0f
        );

        glDrawElements(GL_TRIANGLES, length(cubeIndices), GL_UNSIGNED_INT, 0);
        */


        bagE_swapBuffers();

        t += 0.1;
    }
  
    modelFree(&model);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    glDeleteVertexArrays(1, &modelVao);
    glDeleteBuffers(1, &modelVbo);
    glDeleteBuffers(1, &modelEbo);

    glDeleteProgram(program);
    glDeleteProgram(modelProgram);

    return 0;
}


int bagE_eventHandler(bagE_Event *event)
{
    bool keyDown = false;

    switch (event->type)
    {
        case bagE_EventWindowClose:
            running = 0;
            return 1;

        case bagE_EventWindowResize: {
            bagE_WindowResize *wr = &(event->data.windowResize);
            windowWidth = wr->width;
            windowHeight = wr->height;
            windowChanged = 1;
            glViewport(0, 0, wr->width, wr->height);
        } break;

        case bagE_EventKeyDown: 
            keyDown = true;
        case bagE_EventKeyUp: {
            bagE_Key *key = &(event->data.key);
            switch (key->key) {
                case KEY_A:
                    leftDown = keyDown;
                    break;
                case KEY_D:
                    rightDown = keyDown;
                    break;
                case KEY_W:
                    forthDown = keyDown;
                    break;
                case KEY_S:
                    backDown = keyDown;
                    break;
                case KEY_SPACE:
                    ascendDown = keyDown;
                    break;
                case KEY_SHIFT_LEFT:
                    descendDown = keyDown;
                    break;
                case KEY_ALT_LEFT:
                    if (keyDown) {
                        if (!altDown) {
                            altDown = true;
                            playerInput = !playerInput;
                            bagE_setHiddenCursor(playerInput);
                        }
                    } else {
                        altDown = false;
                    }
                    break;
                case KEY_F:
                    if (keyDown) {
                        if(!fDown) {
                            fDown = true;
                            fullscreen = !fullscreen;
                            bagE_setFullscreen(fullscreen);
                        }
                    } else {
                        fDown = false;
                    }
                    break;
            }
        } break;

        case bagE_EventMouseMotion:
            if (playerInput) {
                bagE_MouseMotion *mm = &(event->data.mouseMotion);
                motionYaw   += mm->x;
                motionPitch += mm->y;
            }
            break;

        default: break;
    }

    return 0;
}
