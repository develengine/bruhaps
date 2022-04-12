#include "state.h"


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
}


void exitState(void)
{

}
