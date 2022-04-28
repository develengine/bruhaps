#include "levels.h"


static const AtlasView atlasViews[] = {
    { 0.0f,  0.75f, 0.25f, 0.25f, 1, 1 },  // grass
    { 0.25f, 0.75f, 0.25f, 0.25f, 1, 1 },  // cobble
};


void levelBruhLoad(void)
{
    level.terrainAtlas = createTexture("res/terrain_atlas.png");

    level.skyboxCubemap = createCubeTexture(
            "Maskonaive2/posx.png",
            "Maskonaive2/negx.png",

            "Maskonaive2/posy.png",
            "Maskonaive2/negy.png",

            "Maskonaive2/posz.png",
            "Maskonaive2/negz.png"
    );

    terrainClearChunkMap(&level.terrain);

    level.atlasViews = atlasViews;


    ChunkHeights *chunkHeights = malloc(sizeof(ChunkHeights));
    malloc_check(chunkHeights);

    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            chunkHeights->data[y * CHUNK_DIM + x] = 
                2.0f * sin((M_PI / CHUNK_DIM) * x * 2.0f) * sin((M_PI / CHUNK_DIM) * y * 3.0f);
        }
    }

    chunkHeights->data[5 * CHUNK_DIM + 8] = NO_TILE;


    ChunkTextures *chunkTextures = malloc(sizeof(ChunkTextures));
    malloc_check(chunkTextures);

    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            TileTexture texture = { 0, 0, 0, 0 };
            chunkTextures->data[y * CHUNK_DIM + x] = texture;
        }
    }

    level.terrain.chunkMap[0] = 0;
    level.terrain.textures[0] = chunkTextures;
    level.terrain.heights[0]  = chunkHeights;
    level.terrain.objects[0]  = createChunkObject();
    level.terrain.chunkCount  = 1;

    level.chunkUpdates[0]  = 0;
    level.chunkUpdateCount = 1;
}


void levelBruhUnload(void)
{
    terrainFreeChunkData(&level.terrain);
    terrainFreeChunkObjects(&level.terrain);

    glDeleteTextures(1, &level.terrainAtlas);
    glDeleteTextures(1, &level.skyboxCubemap);
}


void levelBruhInit(void)
{
}


void levelBruhExit(void)
{
}
