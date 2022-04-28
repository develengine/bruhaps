#ifndef LEVELS_H
#define LEVELS_H

#include "terrain.h"


typedef enum {
    LevelBruh,

    LevelCount
} LevelID;


typedef struct
{
    unsigned terrainProgram;
    unsigned terrainAtlas;
    
    unsigned skyboxProgram;
    unsigned skyboxCubemap;

    const AtlasView *atlasViews;

    Terrain terrain;

    unsigned chunkUpdates[MAX_MAP_DIM * MAX_MAP_DIM];
    int chunkUpdateCount;
} Level;

extern Level level;


void initLevels(void);
void exitLevels(void);

void updateLevel(float dt);
void renderLevel(void);
void renderLevelDebugOverlay(void);

#endif
