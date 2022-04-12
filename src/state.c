#include "state.h"

#include "bag_engine.h"

InputState inputState;
AppState   appState;


void initState(void)
{
    inputState = (InputState) {
        .motionYaw = 0.0f,
        .motionPitch = 0.0f
    };

    appState = (AppState) {
        .running = true
    };

    bagE_getWindowSize(&appState.windowWidth, &appState.windowHeight);
}


void exitState(void)
{

}
