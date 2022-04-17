#ifndef LEVELS_H
#define LEVELS_H

#include "terrain.h"


typedef enum {
    LevelBruh,
} LevelID;


typedef struct
{
    Terrain terrain;
    
    unsigned terrainProgram;
    unsigned terrainAtlas;
    
    unsigned skyboxProgram;
    unsigned skyboxCubemap;
} Level;

extern Level level;


void initLevels(void);
void exitLevels(void);

void updateLevel(float dt);
void renderLevel(void);
void renderLevelDebugOverlay(void);

#endif
