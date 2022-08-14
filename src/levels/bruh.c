#include "game.h"

#define EPS 0.03f

static const AtlasView atlasViews[] = {
    { 0.0f + EPS,  0.75f + EPS, 0.25f - EPS * 2, 0.25f - EPS * 2, 1, 1 },  // grass
    { 0.25f + EPS, 0.75f + EPS, 0.25f - EPS * 2, 0.25f - EPS * 2, 1, 1 },  // cobble
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
    const char *lvlPath = "test.lvl";

    FILE *file = fopen(lvlPath, "rb");
    file_check(file, lvlPath);

    terrainLoad(&level.terrain, file);
    invalidateAllChunks();
    staticsLoad(file);
    spawnersLoad(file);
    pickupsLoad(file);
    
    eof_check(file);

    fclose(file);

    level.filePath = lvlPath;


    // FIXME: should not be hardcoded
    player.x = (CHUNK_DIM + CHUNK_DIM * 1.75) * CHUNK_TILE_DIM;
    player.y = 5.0f;
    player.z = (CHUNK_DIM + CHUNK_DIM * 1.0)  * CHUNK_TILE_DIM;
    camState.pitch = 0.0f;
    camState.yaw   = (float)M_PI;

    if (!gameState.isEditor)
        spawnersBroadcast(SpawnerInit);


    player.selectedGun = Glock;
    player.hp = PLAYER_HP_FULL;
    player.onGround = false;
    player.vy = 0.0f;
    player.won = false;
}


void levelBruhUnload(void)
{
    terrainFreeChunkData(&level.terrain);
    terrainFreeChunkObjects(&level.terrain);
}


void levelBruhInit(void)
{
    level.terrainAtlas = game.defaultTerrainAtlas;
    level.skyboxCubemap = game.defaultSkybox;

    // FIXME: and this
    level.atlasViews = atlasViews;
    level.atlasViewCount = 2;

    for (int i = 0; i < TextureIDCount; ++i)
        levelTextures[i] = createTexture(levelTexturePaths[i]);

    for (int i = 0; i < ModelIDCount; ++i)
        levelModels[i] = loadModelObject(levelModelPaths[i]);

    // FIXME: and also this
    gameInsertStaticObject((Object)   { .model   = levelModels  [ModelTree],
                                        .texture = levelTextures[TextureTree] },
                           (ColliderType) { true, { {{ 0.0f, 1.5f, 0.0f }},
                                                    {{ 0.3f, 1.5f, 0.3f }} } },
                            "Tree");
    gameInsertStaticObject((Object)   { .model   = levelModels  [ModelBush],
                                        .texture = levelTextures[TextureTree] },
                           (ColliderType) { false },
                            "Bush");
    gameInsertStaticObject((Object)   { .model   = levelModels  [ModelRock],
                                        .texture = levelTextures[TextureStone] },
                           (ColliderType) { true, { {{ 0.0f, 0.2f, 0.0f }},
                                                    {{ 0.6f, 0.4f, 0.6f }} } },
                            "Rock");

}


void levelBruhExit(void)
{
    glDeleteTextures(ModelIDCount, levelTextures);

    for (int i = 0; i < ModelIDCount; ++i)
        freeModelObject(levelModels[i]);
}
