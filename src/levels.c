#include "levels.h"

#include "terrain.h"
#include "core.h"
#include "utils.h"

static const AtlasView atlasViews[] = {
    { 0.25f, 0.75f, 0.25f, 0.25f, 1, 1 },  // cobble
    { 0.0f, 0.75f, 0.25f, 0.25f, 1, 1 },   // grass
};

static Map map;
static ChunkHeights chunkHeights;
static ChunkTextures chunkTextures;

static ChunkMesh chunkMesh;
static Model chunkModel;

static ModelObject chunkObject;


static unsigned terrainProgram;
static unsigned grassTexture;


#include "levels/bruh.c"


void initLevels(void)
{
}


void exitLevels(void)
{
}


void updateLevel(float dt)
{
    (void)dt;
}


void renderLevel(void)
{
}

