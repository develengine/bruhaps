#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#define MOUSE_SENSITIVITY 0.005f

typedef struct
{
    bool playerInput;

    bool leftDown;
    bool rightDown;
    bool forthDown;
    bool backDown;
    bool ascendDown;
    bool descendDown;

    bool altDown;
    bool fDown;

    float motionYaw;
    float motionPitch;
} InputState;

extern InputState inputState;


typedef struct
{
    int windowWidth, windowHeight;
    bool running;
    bool fullscreen;
} AppState;

extern AppState appState;


void initState(void);
void exitState(void);

#endif
