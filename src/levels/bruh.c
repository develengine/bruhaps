#include "levels.h"

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
    playerState.x = (CHUNK_DIM + CHUNK_DIM * 1.75) * CHUNK_TILE_DIM;
    playerState.y = 5.0f;
    playerState.z = (CHUNK_DIM + CHUNK_DIM * 1.0)  * CHUNK_TILE_DIM;
    camState.pitch = 0.0f;
    camState.yaw   = (float)M_PI;

    if (!gameState.isEditor)
        spawnersBroadcast(SpawnerInit);


    level.selectedGun = Glock;
    playerState.hp = PLAYER_HP_FULL;
    playerState.onGround = false;
    playerState.vy = 0.0f;
    playerState.won = false;
}


void levelBruhUnload(void)
{
    terrainFreeChunkData(&level.terrain);
    terrainFreeChunkObjects(&level.terrain);
}


void levelBruhInit(void)
{
    // FIXME: this will break when more levels get introduced
    level.terrainAtlas = createTexture("res/terrain_atlas.png");

    // FIXME: so will this
    level.skyboxCubemap = createCubeTexture(
            "res/Maskonaive2/posx.png",
            "res/Maskonaive2/negx.png",

            "res/Maskonaive2/posy.png",
            "res/Maskonaive2/negy.png",

            "res/Maskonaive2/posz.png",
            "res/Maskonaive2/negz.png"
    );

    // FIXME: and this
    level.atlasViews = atlasViews;

    for (int i = 0; i < TextureIDCount; ++i)
        levelTextures[i] = createTexture(levelTexturePaths[i]);

    for (int i = 0; i < ModelIDCount; ++i)
        levelModels[i] = loadModelObject(levelModelPaths[i]);

    // FIXME: and also this
    levelsInsertStaticObject((Object)   { .model   = levelModels  [ModelTree],
                                          .texture = levelTextures[TextureTree] },
                             (ColliderType) { true, { {{ 0.0f, 1.5f, 0.0f }},
                                                      {{ 0.3f, 1.5f, 0.3f }} } });
    levelsInsertStaticObject((Object)   { .model   = levelModels  [ModelBush],
                                          .texture = levelTextures[TextureTree] },
                             (ColliderType) { false });
    levelsInsertStaticObject((Object)   { .model   = levelModels  [ModelRock],
                                          .texture = levelTextures[TextureStone] },
                             (ColliderType) { true, { {{ 0.0f, 0.2f, 0.0f }},
                                                      {{ 0.6f, 0.4f, 0.6f }} } });

}


void levelBruhExit(void)
{
    glDeleteTextures(1, &level.terrainAtlas);
    glDeleteTextures(1, &level.skyboxCubemap);

    glDeleteTextures(ModelIDCount, levelTextures);

    for (int i = 0; i < ModelIDCount; ++i)
        freeModelObject(levelModels[i]);
}
