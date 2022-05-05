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

    level.atlasViews = atlasViews;


    terrainClearChunkMap(&level.terrain);


    ChunkHeights *chunkHeights = malloc(sizeof(ChunkHeights));
    malloc_check(chunkHeights);

    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            chunkHeights->data[y * CHUNK_DIM + x] = 
                2.0f * sinf((float)(M_PI / CHUNK_DIM) * x * 2.0f) * sinf((float)(M_PI / CHUNK_DIM) * y * 3.0f);
        }
    }


    ChunkTextures *chunkTextures = malloc(sizeof(ChunkTextures));
    malloc_check(chunkTextures);

    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            TileTexture texture = { 0, 0, 0, 0 };
            chunkTextures->data[y * CHUNK_DIM + x] = texture;
        }
    }

    int chunkX = 1, chunkZ = 1;
    int chunkPos = chunkZ * MAX_MAP_DIM + chunkX;

    level.terrain.chunkMap[chunkPos] = 0;
    level.terrain.textures[0] = chunkTextures;
    level.terrain.heights[0]  = chunkHeights;
    level.terrain.objects[0]  = createChunkObject();
    level.terrain.chunkCount  = 1;

    requestChunkUpdate(chunkPos);

    playerState.x = 24.0f;
    playerState.y = 5.0f;
    playerState.z = 24.0f;
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
