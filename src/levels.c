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


void initLevels(void)
{
    map = (Map) { 0 };
    memset(map.chunkMap, NO_CHUNK, sizeof(map.chunkMap));
    map.chunkMap[0] = 0;

    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            chunkHeights.data[y * CHUNK_DIM + x] = 
                2.0f * sin((M_PI / CHUNK_DIM) * x * 2.0f) * sin((M_PI / CHUNK_DIM) * y * 3.0f);
        }
    }
    map.heights = &chunkHeights;
    chunkHeights.data[5 * CHUNK_DIM + 8] = NO_TILE;

    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            TileTexture texture = { 0, 0, 0, 0 };
            chunkTextures.data[y * CHUNK_DIM + x] = texture;
        }
    }
    map.textures = &chunkTextures;

    map.chunkCount = 1;

    chunkMesh = (ChunkMesh) { 0 };
    constructChunkMesh(&chunkMesh, &map, atlasViews, 0, 0);
    chunkModel = chunkMeshToModel(chunkMesh);

    chunkObject = createModelObject(chunkModel);


    terrainProgram = createProgram(
            "shaders/terrain_vertex.glsl",
            "shaders/terrain_fragment.glsl"
    );

    grassTexture = createTexture("res/terrain_atlas.png");
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

