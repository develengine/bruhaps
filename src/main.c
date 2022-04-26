#include "bag_engine.h"
#include "bag_keys.h"
#include "utils.h"
#include "linalg.h"
#include "res.h"
#include "animation.h"
#include "terrain.h"
#include "core.h"
#include "levels.h"
#include "state.h"
#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


static bool spinning = true;
static unsigned soundLength;
static int16_t *soundBuffer;


int bagE_main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0);

    printContextInfo();

    bagE_setWindowTitle("BRUHAPS");
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


    initAudio();
    initState();
    initLevels();


    // soundBuffer = loadWAV("test.wav", &soundLength);
    soundBuffer = loadWAV("pushin_d.wav", &soundLength);


    int modelProgram = createProgram("shaders/3d_vertex.glsl", "shaders/3d_fragment.glsl");

    Model brugModel  = modelLoad("res/brug.model");
    ModelObject brug = createModelObject(brugModel);

    Model energyModel  = modelLoad("res/energy.model");
    ModelObject energy = createModelObject(energyModel);

    unsigned brugTexture = createTexture("res/brug.png");

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


#if 0
    unsigned pointProgram = createProgram(
            "shaders/point_vertex.glsl",
            "shaders/point_fragment.glsl"
    );
#endif

    unsigned dummyVao;
    glCreateVertexArrays(1, &dummyVao);

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
#if 0
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

#endif
    /**************************************************/

    unsigned camUBO = createBufferObject(
        sizeof(Matrix) * 3 + sizeof(float) * 4,
        NULL,
        GL_DYNAMIC_STORAGE_BIT
    );

    double t = 0;

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
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camUBO);

    while (appState.running) {
        bagE_pollEvents();

        if (!appState.running)
            break;

        glClearColor(
                0.1f,
                0.2f,
                0.3f,
                1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camState.pitch += inputState.motionPitch * MOUSE_SENSITIVITY;
        camState.yaw   += inputState.motionYaw   * MOUSE_SENSITIVITY;
        inputState.motionPitch = 0.0f;
        inputState.motionYaw   = 0.0f;

        if (inputState.playerInput) {
            if (inputState.leftDown) {
                camState.x -= 0.1f * cosf(camState.yaw);
                camState.z -= 0.1f * sinf(camState.yaw);
            }
            if (inputState.rightDown) {
                camState.x += 0.1f * cosf(camState.yaw);
                camState.z += 0.1f * sinf(camState.yaw);
            }
            if (inputState.forthDown) {
                camState.x += 0.1f * sinf(camState.yaw);
                camState.z -= 0.1f * cosf(camState.yaw);
            }
            if (inputState.backDown) {
                camState.x -= 0.1f * sinf(camState.yaw);
                camState.z += 0.1f * cosf(camState.yaw);
            }
            if (inputState.ascendDown)
                camState.y += 0.1f;
            if (inputState.descendDown)
                camState.y -= 0.1f;
        }

        if (spinning)
            objRotation += 0.025;

        updateLevel(0.01666f);


        Matrix mul;

        /* view */
        Matrix view = matrixTranslation(-camState.x,-camState.y,-camState.z);

        mul = matrixRotationY(camState.yaw);
        view = matrixMultiply(&mul, &view);

        mul = matrixRotationX(camState.pitch);
        view = matrixMultiply(&mul, &view);

        /* projection */
        Matrix proj = matrixProjection(
                camState.fov,
                appState.windowWidth,
                appState.windowHeight,
                0.1f,
                100.0f
        );

        /* vp */
        Matrix vp = matrixMultiply(&proj, &view);

        struct {
            Matrix mats[3];
            float pos[4];
        } camData = {
            { view, proj, vp },
            { camState.x, camState.y, camState.z }
        };
        glNamedBufferSubData(camUBO, 0, sizeof(camData), &camData);


#if 0
        /* model chunk */
        Matrix modelChunk = matrixScale(1.0f, 1.0f, 1.0f);

        glUseProgram(terrainProgram);
        glBindVertexArray(chunkObject.vao);
        glBindTextureUnit(0, grassTexture);

        glProgramUniformMatrix4fv(terrainProgram, 0, 1, GL_FALSE, modelChunk.data);

        glDrawElements(GL_TRIANGLES, chunkModel.indexCount, GL_UNSIGNED_INT, 0);
#endif

        renderLevel();


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

        glProgramUniformMatrix4fv(
                animationProgram,
                3,
                animated.armature.boneCount,
                GL_FALSE,
                (float*)animationMatrices
        );

        glDrawElements(GL_TRIANGLES, animated.model.indexCount, GL_UNSIGNED_INT, 0);


        /* model brug */
        Matrix modelBrug = matrixScale(objScale, objScale, objScale);

        mul = matrixRotationY(objRotation);
        modelBrug = matrixMultiply(&mul, &modelBrug);

        mul = matrixTranslation(objX, objY, objZ);
        modelBrug = matrixMultiply(&mul, &modelBrug);


        glUseProgram(textureProgram);
        glBindVertexArray(brug.vao);
        glBindTextureUnit(0, brugTexture);

        glProgramUniformMatrix4fv(textureProgram, 0, 1, GL_FALSE, modelBrug.data);

        glDrawElements(GL_TRIANGLES, brugModel.indexCount, GL_UNSIGNED_INT, 0);
        

        /* model energy */
        Matrix modelEnergy = matrixScale(objScale, objScale, objScale);

        mul = matrixRotationY(objRotation);
        modelEnergy = matrixMultiply(&mul, &modelEnergy);

        mul = matrixTranslation(objX - 5.0f, objY, objZ);
        modelEnergy = matrixMultiply(&mul, &modelEnergy);


        glUseProgram(textureProgram);
        glBindVertexArray(energy.vao);
        glBindTextureUnit(0, texture);

        glProgramUniformMatrix4fv(textureProgram, 0, 1, GL_FALSE, modelEnergy.data);

        glDrawElements(GL_TRIANGLES, brugModel.indexCount, GL_UNSIGNED_INT, 0);


#if 0
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
#endif

        renderLevelDebugOverlay();


        /* gui */
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(dummyVao);

        glUseProgram(rectProgram);

        glProgramUniform2i(rectProgram, 0, appState.windowWidth, appState.windowHeight);
        glProgramUniform2i(rectProgram, 1, 100, 100);
        glProgramUniform2i(rectProgram, 2, 100, 100);
        glProgramUniform4f(rectProgram, 3, 0.6f, 0.8f, 0.3f, 0.5f);

        glDrawArrays(GL_TRIANGLES, 0, 6);


        /* text */
        glUseProgram(textProgram);
        glBindTextureUnit(0, baseFont);

        glProgramUniform2i(textProgram, 0, appState.windowWidth, appState.windowHeight);
        glProgramUniform2i(textProgram, 1, 100, 8);
        glProgramUniform2i(textProgram, 2, 8, 16);
        glProgramUniform4f(textProgram, 3, 1.0f, 1.0f, 1.0f, 1.0f);

        const char *leText = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do";
        glProgramUniform4uiv(textProgram, 4, 4, (unsigned *)leText);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 63);

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

    glDeleteProgram(modelProgram);
    glDeleteProgram(textureProgram);
    glDeleteProgram(animationProgram);

    exitLevels();
    exitState();
    exitAudio();

    return 0;
}


int bagE_eventHandler(bagE_Event *event)
{
    bool keyDown = false;

    switch (event->type)
    {
        case bagE_EventWindowClose:
            appState.running = false;
            return 1;

        case bagE_EventWindowResize: {
            bagE_WindowResize *wr = &(event->data.windowResize);
            appState.windowWidth = wr->width;
            appState.windowHeight = wr->height;
            glViewport(0, 0, wr->width, wr->height);
        } break;

        case bagE_EventKeyDown: 
            keyDown = true;
            /* fallthrough */
        case bagE_EventKeyUp: {
            bagE_Key *key = &(event->data.key);
            switch (key->key) {
                case KEY_A:
                    inputState.leftDown = keyDown;
                    break;
                case KEY_D:
                    inputState.rightDown = keyDown;
                    break;
                case KEY_W:
                    inputState.forthDown = keyDown;
                    break;
                case KEY_S:
                    inputState.backDown = keyDown;
                    break;
                case KEY_SPACE:
                    inputState.ascendDown = keyDown;
                    break;
                case KEY_SHIFT_LEFT:
                    inputState.descendDown = keyDown;
                    break;
                case KEY_ALT_LEFT:
                    if (keyDown) {
                        if (!inputState.altDown) {
                            inputState.altDown = true;
                            inputState.playerInput = !inputState.playerInput;
                            bagE_setHiddenCursor(inputState.playerInput);
                        }
                    } else {
                        inputState.altDown = false;
                    }
                    break;
                case KEY_F:
                    if (keyDown) {
                        if(!inputState.fDown) {
                            inputState.fDown = true;
                            appState.fullscreen = !appState.fullscreen;
                            bagE_setFullscreen(appState.fullscreen);
                        }
                    } else {
                        inputState.fDown = false;
                    }
                    break;
                case KEY_R:
                    if (keyDown)
                        spinning = !spinning;
                    break;
                case KEY_P:
                    if (!keyDown) {
                        Sound sound = {
                            .data = soundBuffer,
                            .times = 1,
                            .start = 0,
                            .end   = soundLength,
                            .pos   = 0,
                            .volL  = 1.0f,
                            .volR  = 1.0f,
                        };

                        printf("pushin P\n");
                        playSound(sound);
                    }
                    break;
            }
        } break;

        case bagE_EventMouseMotion:
            if (inputState.playerInput) {
                bagE_MouseMotion *mm = &(event->data.mouseMotion);
                inputState.motionYaw   += mm->x;
                inputState.motionPitch += mm->y;
            }
            break;

        case bagE_EventMouseWheel:
            if (inputState.playerInput) {
                bagE_MouseWheel *mw = &(event->data.mouseWheel);
                camState.fov -= mw->scrollUp * 1.0f;
            }
            break;

        default: break;
    }

    return 0;
}
