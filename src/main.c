#include "bag_engine.h"
#include "bag_keys.h"
#include "utils.h"
#include "linalg.h"
#include "res.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

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


static unsigned loadTexture(const char *path)
{
    int width, height, channelCount;

    stbi_set_flip_vertically_on_load(true);

    uint8_t *image = stbi_load(path, &width, &height, &channelCount, STBI_rgb_alpha);
    if (!image) {
        fprintf(stderr, "Failed to load image \"%s\"\n", path);
        exit(1);
    }

    unsigned texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTextureStorage2D(texture, log2(width), GL_RGBA8, width, height);
    glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glGenerateTextureMipmap(texture);

    glBindTexture(GL_TEXTURE_2D, 0);

    free(image);

    return texture;
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


typedef struct
{
    unsigned vao;
    unsigned vbo;
    unsigned ebo;
} ModelObject;


static ModelObject createModelObject(Model model)
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


static void freeModelObject(ModelObject object)
{
    glDeleteVertexArrays(1, &object.vao);
    glDeleteBuffers(1, &object.vbo);
    glDeleteBuffers(1, &object.ebo);
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

static bool spinning = true;


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


    int modelProgram = createProgram("shaders/3d_vertex.glsl", "shaders/3d_fragment.glsl");

    Model brugModel  = modelLoad("res/brug.model");
    ModelObject brug = createModelObject(brugModel);

    Model energyModel  = modelLoad("res/energy.model");
    ModelObject energy = createModelObject(energyModel);

    Model boneModel  = modelLoad("res/bone.model");
    ModelObject bone = createModelObject(boneModel);

    int textureProgram = createProgram(
            "shaders/texture_vertex.glsl",
            "shaders/texture_fragment.glsl"
    );

    unsigned texture = loadTexture("res/monser.png");
    

    Animated animated = animatedLoad("output.bin");


    double t = 0;

    float camX = 0.0f, camY = 0.0f, camZ = 0.0f;
    float camPitch = 0.0f, camYaw = 0.0f;

    float objX, objY, objZ;
    float objScale = 1.0f;
    float objRotation = 0.0f;

    glBindTextureUnit(0, texture);
    glActiveTexture(GL_TEXTURE0);

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

        if (spinning)
            objRotation += 0.025;


        Matrix mul;

        /* view */
        Matrix view = matrixTranslation(-camX,-camY,-camZ);

        mul = matrixRotationY(camYaw);
        view = matrixMultiply(&mul, &view);

        mul = matrixRotationX(camPitch);
        view = matrixMultiply(&mul, &view);

        /* projection */
        Matrix proj = matrixProjection(90.0f, windowWidth, windowHeight, 0.1f, 100.0f);

        /* vp */
        Matrix vp = matrixMultiply(&proj, &view);


        /* model brug */
        Matrix modelBrug = matrixScale(objScale, objScale, objScale);

        mul = matrixRotationY(objRotation);
        modelBrug = matrixMultiply(&mul, &modelBrug);

        mul = matrixTranslation(objX, objY, objZ);
        modelBrug = matrixMultiply(&mul, &modelBrug);


        glUseProgram(modelProgram);
        glBindVertexArray(brug.vao);

        glProgramUniformMatrix4fv(modelProgram, 0, 1, GL_FALSE, vp.data);
        glProgramUniformMatrix4fv(modelProgram, 1, 1, GL_FALSE, modelBrug.data);
        glProgramUniform3f(modelProgram, 2, camX, camY, camZ);
        glProgramUniform3f(modelProgram, 3, 0.75f, 0.75f, 0.75f);

        glDrawElements(GL_TRIANGLES, brugModel.indexCount, GL_UNSIGNED_INT, 0);


        /* model bone */
        glBindVertexArray(bone.vao);

        Matrix modelBone = matrixTranslation(0.0f, 0.0f, 0.0f);

        glProgramUniformMatrix4fv(modelProgram, 0, 1, GL_FALSE, vp.data);
        glProgramUniformMatrix4fv(modelProgram, 1, 1, GL_FALSE, modelBone.data);
        glProgramUniform3f(modelProgram, 2, camX, camY, camZ);
        glProgramUniform3f(modelProgram, 3, 0.75f, 0.75f, 0.75f);

        glDrawElements(GL_TRIANGLES, boneModel.indexCount, GL_UNSIGNED_INT, 0);


        /* model energy */
        Matrix modelEnergy = matrixScale(objScale, objScale, objScale);

        mul = matrixRotationY(objRotation);
        modelEnergy = matrixMultiply(&mul, &modelEnergy);

        mul = matrixTranslation(objX - 5.0f, objY, objZ);
        modelEnergy = matrixMultiply(&mul, &modelEnergy);


        glUseProgram(textureProgram);
        glBindVertexArray(energy.vao);

        glProgramUniformMatrix4fv(textureProgram, 0, 1, GL_FALSE, vp.data);
        glProgramUniformMatrix4fv(textureProgram, 1, 1, GL_FALSE, modelEnergy.data);
        glProgramUniform3f(textureProgram, 2, camX, camY, camZ);
        glProgramUniform3f(textureProgram, 3, 0.75f, 0.75f, 0.75f);

        glDrawElements(GL_TRIANGLES, brugModel.indexCount, GL_UNSIGNED_INT, 0);


        /* model cube */
        Matrix modelCube = matrixScale(objScale, objScale, objScale);

        mul = matrixRotationY(objRotation);
        modelCube = matrixMultiply(&mul, &modelCube);

        mul = matrixTranslation(objX + 5.0f, objY, objZ);
        modelCube = matrixMultiply(&mul, &modelCube);

        /* mvp */
        Matrix mvp = matrixMultiply(&view, &modelCube);
        mvp = matrixMultiply(&proj, &mvp);


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


        bagE_swapBuffers();

        t += 0.1;
    }
  
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    animatedFree(animated);

    modelFree(boneModel);
    freeModelObject(bone);

    modelFree(brugModel);
    freeModelObject(brug);

    modelFree(energyModel);
    freeModelObject(energy);

    glDeleteTextures(1, &texture);

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
                case KEY_R:
                    if (keyDown)
                        spinning = !spinning;
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
