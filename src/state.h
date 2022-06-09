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


typedef struct
{
    bool inSplash;
    bool isEditor;
} GameState;

extern GameState gameState;


typedef struct
{
    float x, y, z;
    float pitch, yaw;
    float fov;
} CamState;

extern CamState camState;


typedef struct
{
    bool gaming;
    bool onGround;
    float x, y, z;
    float vy;
    bool tryJump;
    int hp;
} PlayerState;

extern PlayerState playerState;


void initState(void);
void exitState(void);

#endif
