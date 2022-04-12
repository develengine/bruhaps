#include "bag_engine.h"
#include "bag_keys.h"
#include "utils.h"
#include "linalg.h"
#include "res.h"
#include "animation.h"
#include "terrain.h"
#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


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

static int boneShow = 1;

static float fov = 90.0f;


static bool selected = false;
static int selectedX, selectedZ;

static void selectVertex(
        float camX,
        float camY,
        float camZ,
        float camPitch,
        float camYaw,
        Map *map
) {
    float vx = sinf(camYaw), vz = -cosf(camYaw);
    float camXS = (camX) / CHUNK_TILE_DIM + 0.5f;
    float camZS = (camZ) / CHUNK_TILE_DIM + 0.5f;
    float x = camXS;
    float z = camZS;
    float yx = -sinf(camPitch) / (cosf(camPitch));

    selected = false;

    for (int i = 0; i < 10; ++i) {
        float gx = vx > 0.0f ? ceilf(x + 0.01f) : floorf(x - 0.01f);
        float gz = vz > 0.0f ? ceilf(z + 0.01f) : floorf(z - 0.01f);

        float rx = (gx - x) / vx;
        float rz = (gz - z) / vz;

        if (rx < rz) {
            z = z + rx * vz;
            x = gx;
        } else {
            x = x + rz * vx;
            z = gz;
        }

        int xp = floor(x);
        int zp = floor(z);
        int cx = xp / CHUNK_DIM;
        int cz = zp / CHUNK_DIM;

        if (xp >= 0 && cx < MAX_MAP_DIM
         && zp >= 0 && cz < MAX_MAP_DIM
         && map->chunkMap[cz * MAX_MAP_DIM + cx] != NO_CHUNK) {
            int dx = x - camXS;
            int dz = z - camZS;
            float h = camY + sqrtf(dx * dx + dz * dz) * yx;

            int lx = xp % CHUNK_DIM;
            int lz = zp % CHUNK_DIM;

            if (h < map->heights[map->chunkMap[cz * MAX_MAP_DIM + cx]].data[lz * CHUNK_DIM + lx]) {
                selected = true;
                selectedX = xp;
                selectedZ = zp;
                return;
            }
        }
    }
}


int bagE_main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("num: %d\n", '!');

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
    glEnable(GL_PROGRAM_POINT_SIZE);


    int program = createProgram("shaders/proper_vert.glsl", "shaders/proper_frag.glsl");


    ModelObject cubeModel = createCubeModelObject();


    int modelProgram = createProgram("shaders/3d_vertex.glsl", "shaders/3d_fragment.glsl");

    Model brugModel  = modelLoad("res/brug.model");
    ModelObject brug = createModelObject(brugModel);

    Model energyModel  = modelLoad("res/energy.model");
    ModelObject energy = createModelObject(energyModel);

    int textureProgram = createProgram(
            "shaders/texture_vertex.glsl",
            "shaders/texture_fragment.glsl"
    );

    unsigned texture = createTexture("res/monser.png");
    

    Animated animated = animatedLoad("output.bin");
    AnimatedObject animatedObject = createAnimatedObject(animated);

    Matrix animationMatrices[64];
    JointTransform animationTransforms[64];

    unsigned animationProgram = createProgram(
            "shaders/animated_vertex.glsl",
            "shaders/animated_fragment.glsl"
    );

    unsigned wormTexture = createTexture("res/worm.png");


    unsigned cubeMap = createCubeTexture(
            "Maskonaive2/posx.png",
            "Maskonaive2/negx.png",

            "Maskonaive2/posy.png",
            "Maskonaive2/negy.png",

            "Maskonaive2/posz.png",
            "Maskonaive2/negz.png"
    );

    unsigned cubeProgram = createProgram(
            "shaders/cubemap_vertex.glsl",
            "shaders/cubemap_fragment.glsl"
    );

    unsigned terrainProgram = createProgram(
            "shaders/terrain_vertex.glsl",
            "shaders/terrain_fragment.glsl"
    );

    unsigned grassTexture = createTexture("res/terrain_atlas.png");

    unsigned pointProgram = createProgram(
            "shaders/point_vertex.glsl",
            "shaders/point_fragment.glsl"
    );
    unsigned dummyVao;
    glCreateVertexArrays(1, &dummyVao);

    ModelObject boxModel = createBoxModelObject();

    unsigned rectProgram = createProgram(
            "shaders/rect_vertex.glsl",
            "shaders/rect_fragment.glsl"
    );

    unsigned baseFont = createTexture("res/font_base.png");
    unsigned textProgram = createProgram(
            "shaders/text_vertex.glsl",
            "shaders/text_fragment.glsl"
    );

    /**************************************************/

    Map map = { 0 };
    memset(map.chunkMap, NO_CHUNK, sizeof(map.chunkMap));
    map.chunkMap[0] = 0;

    ChunkHeights chunkHeights;
    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            chunkHeights.data[y * CHUNK_DIM + x] = 
                2.0f * sin((M_PI / CHUNK_DIM) * x * 2.0f) * sin((M_PI / CHUNK_DIM) * y * 3.0f);
        }
    }
    map.heights = &chunkHeights;
    chunkHeights.data[5 * CHUNK_DIM + 8] = NO_TILE;

    ChunkTextures chunkTextures;
    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            TileTexture texture = { 0, 0, 0, 0 };
            chunkTextures.data[y * CHUNK_DIM + x] = texture;
        }
    }
    map.textures = &chunkTextures;

    AtlasView atlasViews[] = {
        { 0.25f, 0.75f, 0.25f, 0.25f, 1, 1 },  // cobble
        { 0.0f, 0.75f, 0.25f, 0.25f, 1, 1 },   // grass
    };

    map.chunkCount = 1;

    ChunkMesh chunkMesh = { 0 };
    constructChunkMesh(&chunkMesh, &map, atlasViews, 0, 0);
    Model chunkModel = chunkMeshToModel(chunkMesh);

    ModelObject chunkObject = createModelObject(chunkModel);

    /**************************************************/

    double t = 0;

    float camX = 0.0f, camY = 0.0f, camZ = 0.0f;
    float camPitch = 0.0f, camYaw = 0.0f;

    float objX = 0.0f;
    float objY = 0.0f;
    float objZ = -10.0f;

    float objScale = 1.0f;
    float objRotation = 0.0f;

    Animation animation = {
        .start = animated.armature.timeStamps[0],
        .end   = animated.armature.timeStamps[2],
        .time  = 0.0f
    };

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

        selectVertex(camX, camY, camZ, camPitch, camYaw, &map);

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
        Matrix proj = matrixProjection(fov, windowWidth, windowHeight, 0.1f, 100.0f);

        /* vp */
        Matrix vp = matrixMultiply(&proj, &view);


        /* cube map */
        Matrix envView = matrixRotationY(camYaw);
        mul = matrixRotationX(camPitch);
        envView = matrixMultiply(&mul, &envView);

        glDisable(GL_DEPTH_TEST);

        glUseProgram(cubeProgram);
        glBindVertexArray(boxModel.vao);
        glBindTextureUnit(0, cubeMap);

        glProgramUniformMatrix4fv(cubeProgram, 0, 1, GL_FALSE, view.data);
        glProgramUniformMatrix4fv(cubeProgram, 1, 1, GL_FALSE, proj.data);

        glDrawElements(GL_TRIANGLES, BOX_INDEX_COUNT, GL_UNSIGNED_INT, 0);

        glEnable(GL_DEPTH_TEST);


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

        
        /* model chunk */
        Matrix modelChunk = matrixScale(1.0f, 1.0f, 1.0f);

        glUseProgram(terrainProgram);
        glBindVertexArray(chunkObject.vao);
        glBindTextureUnit(0, grassTexture);

        glProgramUniformMatrix4fv(terrainProgram, 0, 1, GL_FALSE, vp.data);
        glProgramUniformMatrix4fv(terrainProgram, 1, 1, GL_FALSE, modelChunk.data);
        glProgramUniform3f(terrainProgram, 3, 0.5f, 1.0f, 0.5f);

        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, chunkModel.indexCount, GL_UNSIGNED_INT, 0);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


        /* animated model */
        glUseProgram(animationProgram);
        glBindVertexArray(animatedObject.model.vao);
        glBindTextureUnit(0, wormTexture);

        updateAnimation(&animation, 0.01666f);
        computePoseTransforms(&animated.armature, animationTransforms, animation.time);

        Matrix modelBone = matrixRotationY(objRotation);
        mul = matrixTranslation(0.0f, 0.0f,-2.0f);
        modelBone = matrixMultiply(&mul, &modelBone);

        computeArmatureMatrices(
                modelBone,
                animationMatrices,
                animationTransforms,
                &animated.armature,
                0
        );

        glProgramUniformMatrix4fv(animationProgram, 0, 1, GL_FALSE, vp.data);
        glProgramUniformMatrix4fv(
                animationProgram,
                3,
                animated.armature.boneCount,
                GL_FALSE,
                (float*)animationMatrices
        );
        glProgramUniform3f(animationProgram, 1, camX, camY, camZ);
        glProgramUniform3f(animationProgram, 2, 0.75f, 0.75f, 0.75f);

        glDrawElements(GL_TRIANGLES, animated.model.indexCount, GL_UNSIGNED_INT, 0);


        /* model energy */
        Matrix modelEnergy = matrixScale(objScale, objScale, objScale);

        mul = matrixRotationY(objRotation);
        modelEnergy = matrixMultiply(&mul, &modelEnergy);

        mul = matrixTranslation(objX - 5.0f, objY, objZ);
        modelEnergy = matrixMultiply(&mul, &modelEnergy);


        glUseProgram(textureProgram);
        glBindVertexArray(energy.vao);
        glBindTextureUnit(0, texture);

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


        glBindVertexArray(cubeModel.vao);
        glUseProgram(program);

        glProgramUniformMatrix4fv(program, 0, 1, GL_FALSE, mvp.data);
        glProgramUniform4f(program, 1,
                0.1f,
                (sin(t) + 1.0) * 0.5,
                0.5f,
                1.0f
        );

        glDrawElements(GL_TRIANGLES, BOX_INDEX_COUNT, GL_UNSIGNED_INT, 0);

        /* point */
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(dummyVao);

        if (selected) {
            glUseProgram(pointProgram);

            glProgramUniformMatrix4fv(pointProgram, 0, 1, GL_FALSE, vp.data);
            glProgramUniform4f(
                    pointProgram,
                    1,
                    selectedX * CHUNK_TILE_DIM,
                    atMapHeight(&map, selectedX, selectedZ),
                    selectedZ * CHUNK_TILE_DIM,
                    1.0f
            );
            glProgramUniform4f(pointProgram, 2, 0.5f, 1.0f, 0.75f, 1.0f);
            glProgramUniform1f(pointProgram, 3, 18.0f);

            glDrawArrays(GL_POINTS, 0, 1);
        }

        /* gui */
        glUseProgram(rectProgram);

        glProgramUniform2i(rectProgram, 0, windowWidth, windowHeight);
        glProgramUniform2i(rectProgram, 1, 100, 100);
        glProgramUniform2i(rectProgram, 2, 100, 100);
        glProgramUniform4f(rectProgram, 3, 0.6f, 0.8f, 0.3f, 0.5f);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        /* text */
        glUseProgram(textProgram);
        glBindTextureUnit(0, baseFont);

        glProgramUniform2i(textProgram, 0, windowWidth, windowHeight);
        glProgramUniform2i(textProgram, 1, 300, 300);
        glProgramUniform2i(textProgram, 2, 8 * 8, 16 * 8);
        glProgramUniform4f(textProgram, 3, 1.0f, 1.0f, 1.0f, 1.0f);

        const char *leText = "lol cool k\0\0\0\0\0\0";
        glProgramUniform4uiv(textProgram, 4, 1, (unsigned *)leText);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 10);

        glEnable(GL_DEPTH_TEST);


        bagE_swapBuffers();

        t += 0.1;
    }
  
    animatedFree(animated);

    modelFree(brugModel);
    freeModelObject(brug);

    modelFree(energyModel);
    freeModelObject(energy);

    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &wormTexture);
    glDeleteTextures(1, &cubeMap);

    glDeleteProgram(program);
    glDeleteProgram(modelProgram);
    glDeleteProgram(textureProgram);
    glDeleteProgram(animationProgram);
    glDeleteProgram(cubeProgram);

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
            /* fallthrough */
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

                case KEY_B:
                    if (keyDown)
                        ++boneShow;
                    break;
                case KEY_N:
                    if (keyDown)
                        --boneShow;
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
