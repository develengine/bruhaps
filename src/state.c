#include "state.h"

#include "bag_engine.h"

InputState  inputState;
AppState    appState;
CamState    camState;
PlayerState playerState;


void initState(void)
{
    inputState = (InputState) {
        .motionYaw = 0.0f,
        .motionPitch = 0.0f
    };

    appState = (AppState) {
        .running = true
    };

    camState = (CamState) {
        .x = 0.0f, .y = 0.0f, .z = 0.0f,
        .pitch = 0.0f, .yaw = 0.0f,
        .fov = 90.0f
    };

    playerState = (PlayerState) {
        .gaming = true,
        .onGround = false,
        .x = 0.0f, .y = 0.0f, .z = 0.0f,
        .ay = 0.0f
    };

    bagE_getWindowSize(&appState.windowWidth, &appState.windowHeight);
}


void exitState(void)
{

}
