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
#include "gui.h"
#include "splash.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


int bagE_main(int argc, char *argv[])
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0);

    printContextInfo();

    bagE_setWindowTitle("BRUHAPS");

    // TODO: for now the game is frame dependent
    //       swap interval must be set to 1
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
    // glEnable(GL_FRAMEBUFFER_SRGB);


    initAudio();
    initState();

    if (argc > 1)
        gameState.isEditor = strcmp(argv[1], "--editor") == 0;

    initGUI();
    initLevels();
    initSplash();


    Model energyModel  = modelLoad("res/energy.model");
    ModelObject energy = createModelObject(energyModel);
    modelFree(energyModel);

    int textureProgram = createProgram(
            "shaders/texture_vertex.glsl",
            "shaders/texture_fragment.glsl"
    );

    unsigned texture = createTexture("res/monser.png");
    

    unsigned camUBO = createBufferObject(
        sizeof(Matrix) * 3 + sizeof(float) * 4,
        NULL,
        GL_DYNAMIC_STORAGE_BIT
    );

    unsigned envUBO = createBufferObject(
        sizeof(float) * 4 * 4,
        NULL,
        GL_DYNAMIC_STORAGE_BIT
    );


    double t = 0;

    float objX = 0.0f;
    float objY = 0.0f;
    float objZ = -10.0f;

    float objScale = 1.0f;
    float objRotation = 0.0f;

    glActiveTexture(GL_TEXTURE0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, envUBO);


    struct {
        float ambient[4];
        float toLight[4];
        float sunColor[4];
    } envData = {
        { 0.1f, 0.2f, 0.3f, 1.0f },
        { 0.6f, 0.7f, 0.5f },
        { 1.0f, 1.0f, 1.0f },
    };
    glNamedBufferSubData(envUBO, 0, sizeof(envData), &envData);


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

        if (inputState.playerInput && !gameState.inSplash) {
            float vx = 0.0f, vz = 0.0f;

            if (inputState.leftDown) {
                vx -= 0.1f * cosf(camState.yaw);
                vz -= 0.1f * sinf(camState.yaw);
            }
            if (inputState.rightDown) {
                vx += 0.1f * cosf(camState.yaw);
                vz += 0.1f * sinf(camState.yaw);
            }
            if (inputState.forthDown) {
                vx += 0.1f * sinf(camState.yaw);
                vz -= 0.1f * cosf(camState.yaw);
            }
            if (inputState.backDown) {
                vx -= 0.1f * sinf(camState.yaw);
                vz += 0.1f * cosf(camState.yaw);
            }

            if (playerState.gaming) {
                processPlayerInput(vx, vz, playerState.tryJump && inputState.ascendDown, 0.01666f);

                playerState.tryJump = !inputState.ascendDown;

                camState.x = playerState.x;
                camState.y = playerState.y;
                camState.z = playerState.z;
            } else {
                if (inputState.ascendDown)
                    camState.y += 0.1f;
                if (inputState.descendDown)
                    camState.y -= 0.1f;

                camState.x += vx;
                camState.z += vz;
            }
        }

        objRotation += 0.025f;


        if (gameState.inSplash) {
            updateSplash(0.01666f);
        } else {
            updateLevel(0.01666f);
        }


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
                (float)appState.windowWidth,
                (float)appState.windowHeight,
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


        if (gameState.inSplash) {
            renderSplash();
        } else {
            renderLevel();
        }


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

        glDrawElements(GL_TRIANGLES, energy.indexCount, GL_UNSIGNED_INT, 0);


        /* overlay */
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(gui.dummyVao);

        guiUpdateResolution(appState.windowWidth, appState.windowHeight);

        if (gameState.inSplash) {
            renderSplashOverlay();
        } else {
            renderLevelOverlay();
        }

        glEnable(GL_DEPTH_TEST);


        bagE_swapBuffers();

        t += 0.1;
    }
  
    freeModelObject(energy);

    glDeleteTextures(1, &texture);

    glDeleteProgram(textureProgram);

    exitSplash();
    exitLevels();
    exitGUI();
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
                case KEY_L:
                    if (!keyDown && !gameState.inSplash)
                        levelsSaveCurrent();
                    break;
            }
        } break;

        case bagE_EventMouseMotion:
            if (inputState.playerInput && !gameState.inSplash) {
                bagE_MouseMotion *mm = &(event->data.mouseMotion);
                inputState.motionYaw   += mm->x;
                inputState.motionPitch += mm->y;
            }
            break;

        case bagE_EventMousePosition:
            if (gameState.inSplash) {
                bagE_Mouse *m = &(event->data.mouse);
                splashProcessMouse(m);
            }
            break;

        case bagE_EventMouseButtonDown:
            keyDown = true;
            /* fallthrough */
        case bagE_EventMouseButtonUp: {
                bagE_MouseButton *mb = &(event->data.mouseButton);
                if (gameState.inSplash) {
                    splashProcessButton(mb);
                } else {
                    levelsProcessButton(mb, keyDown);
                }
            }
            break;

        case bagE_EventMouseWheel:
            if (inputState.playerInput) {
                bagE_MouseWheel *mw = &(event->data.mouseWheel);
                // camState.fov -= mw->scrollUp * 1.0f;
                if (!gameState.inSplash)
                    levelsProcessWheel(mw);
            }
            break;

        default: break;
    }

    return 0;
}
