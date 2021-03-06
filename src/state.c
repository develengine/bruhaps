#include "state.h"

#include "bag_engine.h"

InputState  inputState;
AppState    appState;
GameState   gameState;
CamState    camState;
PlayerState playerState;


void initState(void)
{
    inputState = (InputState) {
        .motionYaw = 0.0f,
        .motionPitch = 0.0f
    };

    appState = (AppState) {
        .running = true,
        .volume  = 1.0f
    };

    gameState = (GameState) {
        .inSplash = true,
        .isEditor = false,
        .isPaused = false,
        .sensitivity = 0.005f
    };

    camState = (CamState) {
        .x = 0.0f, .y = 0.0f, .z = 0.0f,
        // COPE: Slight offset so that skybox doesn't mess up.
        .pitch = 0.001f, .yaw = 0.001f,
        .fov = 90.0f
    };

    playerState = (PlayerState) {
        .gaming = false,
        .onGround = false,
        .x = 0.0f, .y = 0.0f, .z = 0.0f,
        .vy = 0.0f,
        .tryJump = true
    };

    bagE_getWindowSize(&appState.windowWidth, &appState.windowHeight);
}


void exitState(void)
{

}
