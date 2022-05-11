#ifndef LEVELS_H
#define LEVELS_H

#include "terrain.h"

#include "bag_engine.h"
#include "core.h"

#include <stdbool.h>


typedef enum
{
    LevelBruh,

    LevelCount
} LevelID;


#define MAX_STATIC_TYPE_COUNT 128
/* NOTE: reflected in shader/static_vertex.glsl */
#define MAX_STATIC_INSTANCE_COUNT 1024


typedef struct
{
    float x, y, z;
    float scale;
    float rx, ry, rz;
    float padding;
} ModelTransform;


typedef struct
{
    bool exists;
    float x, y, z;
    float sx, sy, sz;
    float rx;
} Collider;


typedef struct
{
    ModelObject model;
    unsigned texture;
} Object;


typedef struct
{
    unsigned terrainProgram;
    unsigned terrainAtlas;
    
    unsigned skyboxProgram;
    unsigned skyboxCubemap;

    const AtlasView *atlasViews;

    const char *filePath;

    Terrain terrain;

    int chunkUpdateCount;
    unsigned chunkUpdates[MAX_MAP_DIM * MAX_MAP_DIM];

    int statsTypeCount;
    int      statsOffsets  [MAX_STATIC_TYPE_COUNT];
    Object   statsObjects  [MAX_STATIC_TYPE_COUNT];
    Collider statsColliders[MAX_STATIC_TYPE_COUNT];

    bool recalculateStats;
    int statsInstanceCount;
    ModelTransform statsTransforms[MAX_STATIC_INSTANCE_COUNT];
} Level;

extern Level level;


static inline int getStaticCount(int staticID)
{
    return level.statsOffsets[staticID + 1] - level.statsOffsets[staticID];
}


void initLevels(void);
void exitLevels(void);

void processPlayerInput(float vx, float vz, bool jump, float dt);

void updateLevel(float dt);
void renderLevel(void);
void renderLevelDebugOverlay(void);

void levelsSaveCurrent(void);

void requestChunkUpdate(unsigned chunkPos);
void invalidateAllChunks(void);

void levelsProcessButton(bagE_MouseButton *mb);
void levelsProcessWheel(bagE_MouseWheel *mw);

void levelsInsertStaticObject(Object object, Collider collider);
void levelsAddStatic(int statID, ModelTransform transform);

#endif
