#include "levels.h"


static const AtlasView atlasViews[] = {
    { 0.0f,  0.75f, 0.25f, 0.25f, 1, 1 },  // grass
    { 0.25f, 0.75f, 0.25f, 0.25f, 1, 1 },  // cobble
};


typedef enum
{
    TextureTree,
    TextureStone,
    TextureGrass,

    TextureIDCount
} TextureID;

static const char *levelTexturePaths[TextureIDCount] = {
    [TextureTree]  = "res/tree1.png",
    [TextureStone] = "res/stone.png",
    [TextureGrass] = "res/grass_patch.png",
};


typedef enum
{
    ModelTree,
    ModelBush,
    ModelRock,
    ModelGrass,

    ModelIDCount
} ModelID;

static const char *levelModelPaths[ModelIDCount] = {
    [ModelTree]  = "res/tree.model",
    [ModelBush]  = "res/bush.model",
    [ModelRock]  = "res/rock.model",
    [ModelGrass] = "res/grass.model",
};


static unsigned    levelTextures[TextureIDCount];
static ModelObject levelModels  [ModelIDCount];


void levelBruhLoad(void)
{
    level.terrainAtlas = createTexture("res/terrain_atlas.png");

    level.skyboxCubemap = createCubeTexture(
            "res/Maskonaive2/posx.png",
            "res/Maskonaive2/negx.png",

            "res/Maskonaive2/posy.png",
            "res/Maskonaive2/negy.png",

            "res/Maskonaive2/posz.png",
            "res/Maskonaive2/negz.png"
    );

    level.atlasViews = atlasViews;

    const char *lvlPath = "test.lvl";

#if 0
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


    FILE *file = fopen(lvlPath, "wb");
    file_check(file, lvlPath);

    terrainSave(&level.terrain, file);

    fclose(file);
#else
    FILE *file = fopen(lvlPath, "rb");
    file_check(file, lvlPath);

    terrainLoad(&level.terrain, file);
    invalidateAllChunks();
    staticsLoad(file);
    
    eof_check(file);

    fclose(file);
#endif

    level.filePath = lvlPath;


    for (int i = 0; i < TextureIDCount; ++i)
        levelTextures[i] = createTexture(levelTexturePaths[i]);

    for (int i = 0; i < ModelIDCount; ++i)
        levelModels[i] = loadModelObject(levelModelPaths[i]);

    levelsInsertStaticObject((Object)   { .model   = levelModels  [ModelTree],
                                          .texture = levelTextures[TextureTree] },
                             (ColliderType) { true, { { 0.0f, 1.5f, 0.0f },
                                                      { 0.3f, 1.5f, 0.3f } } });
    levelsInsertStaticObject((Object)   { .model   = levelModels  [ModelBush],
                                          .texture = levelTextures[TextureTree] },
                             (ColliderType) { false });
    levelsInsertStaticObject((Object)   { .model   = levelModels  [ModelRock],
                                          .texture = levelTextures[TextureStone] },
                             (ColliderType) { true, { 0.0f, 0.2f, 0.0f,
                                                      0.6f, 0.4f, 0.6f } });
    /*
    levelsInsertStaticObject((Object)   { .model   = levelModels  [ModelGrass],
                                          .texture = levelTextures[TextureGrass] },
                             (ColliderType) { false });
    */


    playerState.x = (CHUNK_DIM + CHUNK_DIM * 0.5) * CHUNK_TILE_DIM;
    playerState.y = 5.0f;
    playerState.z = (CHUNK_DIM + CHUNK_DIM * 0.5) * CHUNK_TILE_DIM;
}


void levelBruhUnload(void)
{
    terrainFreeChunkData(&level.terrain);
    terrainFreeChunkObjects(&level.terrain);

    glDeleteTextures(1, &level.terrainAtlas);
    glDeleteTextures(1, &level.skyboxCubemap);

    glDeleteTextures(ModelIDCount, levelTextures);

    for (int i = 0; i < ModelIDCount; ++i)
        freeModelObject(levelModels[i]);
}


void levelBruhInit(void)
{
}


void levelBruhExit(void)
{
}
