#include "levels.h"

#include "terrain.h"
#include "core.h"
#include "utils.h"
#include "state.h"
#include "audio.h"
#include "gui.h"
#include "settings.h"


PlayerState playerState;
Level level;


const char *spawnerGroupNames[] = {
    [SpawnerInit] = "On init"
};

static_assert(length(spawnerGroupNames) == SpawnerGroupCount,
              "unfilled spawner group name");


int mobStartingHPs[] = {
    [MobWorm] = 100,
};

static_assert(length(mobStartingHPs) == MobCount,
              "unfilled mob starting HPs");


float pickupScales[] = {
    [HealthPickup] = 2.0f,
    [AmmoPickup]   = 0.5f,
    [HeadPickup]   = 1.0f,
};

static_assert(length(pickupScales) == PickupCount,
              "unfilled pickup scales");


bool pickupHealth(void)
{
    if (playerState.hp < PLAYER_HP_FULL) {
        playerState.hp += 50;

        if (playerState.hp > PLAYER_HP_FULL)
            playerState.hp = PLAYER_HP_FULL;

        playSound((Sound) {
            .data  = level.gulp,
            .end   = level.gulpLength,
            .volL  = 0.5f,
            .volR  = 0.5f,
            .times = 1,
        });

        return true;
    }

    return false;
}


bool pickupAmmo(void)
{
    level.gatlingAmmo += 50;

    playSound((Sound) {
        .data  = level.ssAmmoPickup,
        .end   = level.ssAmmoPickupLength,
        .volL  = 0.5f,
        .volR  = 0.5f,
        .times = 1,
    });

    return true;
}


bool pickupHead(void)
{
    ++level.carryHeadCount;

    playSound((Sound) {
        .data  = level.bruh2,
        .end   = level.bruh2Length,
        .volL  = 0.5f,
        .volR  = 0.5f,
        .times = 1,
    });

    return true;
}


PickupAction pickupActions[] = {
    [HealthPickup] = pickupHealth,
    [AmmoPickup]   = pickupAmmo,
    [HeadPickup]   = pickupHead,
};

static_assert(length(pickupActions) == PickupCount,
              "unfilled pickup action");


static unsigned pointProgram;


typedef enum
{
    TerrainPlacing,
    TerrainTexturing,
    // TerrainHeightPaintingAbs,
    TerrainHeightPaintingRel,
    StaticsPlacing,
    SpawnerPlacing,
    PickupPlacing,

    EditorModeCount
} EditorMode;

#define NoEditorMode EditorModeCount

static EditorMode editorMode = TerrainHeightPaintingRel;


static bool selected = false;
static int selectedX, selectedZ;
static int brushWidth = 3;

#define MAX_BRUSH_WIDTH 3

static uint8_t selectedViewID = 1;
// static uint8_t selectedViewTrans = 0;

static float selectedRelHeight = 0.1f;
// static float selectedAbsHeight = 3.0f;

static int selectedStaticID = 0;

static MobType selectedSpawnerType = MobWorm;

static Pickup selectedPickup = HeadPickup;

static int terrainHeightDown = false;

static void scaleHeights(float scale);


static const char *editorModeNames[] = {
    [TerrainPlacing]           = " Terrain Placing ",
    [TerrainTexturing]         = "Terrain Texturing",
    [TerrainHeightPaintingRel] = " Height Painting ",
    [StaticsPlacing]           = " Statics Placing ",
    [SpawnerPlacing]           = " Spawner Placing ",
    [PickupPlacing]            = " Pickups Placing ",
};

static EditorMode selectedEditorMode = NoEditorMode;

#define EDITOR_FONT_SIZE 32

typedef enum
{
    EditorMainMenu,
    EditorSaveLevel,

    EditorButtonCount
} EditorButtonID;

#define EditorNoButton EditorButtonCount

static EditorButtonID selectedEditorButton = EditorNoButton;

static const char *editorButtonNames[EditorButtonCount] = {
    [EditorMainMenu]  = "Main Menu ",
    [EditorSaveLevel] = "Save Level",
};

static_assert(length(editorButtonNames) == EditorButtonCount,
              "unfilled editor button name");


#define TEXT_MUL 4

typedef enum
{
    PauseResume,
    PauseSettings,
    PauseMainMenu,

    PauseButtonCount
} PauseButtonID;

#define NO_BUTTON (PauseButtonCount + 1)

static const char *pauseButtonNames[] = {
    [PauseResume]   = "  RESUME  ",
    [PauseSettings] = " SETTINGS ",
    [PauseMainMenu] = "   MENU   ",
};

static_assert(length(pauseButtonNames) == PauseButtonCount,
              "unfilled button name");

typedef enum
{
    PauseBaseState,
    PauseSettingsState,
} PauseState;

static PauseState pauseState = PauseBaseState;


static void onPauseResume(void)
{
    inputState.playerInput = true;
    gameState.isPaused = false;
    bagE_setHiddenCursor(true);
}

static void onPauseSettings(void)
{
    pauseState = PauseSettingsState;
}

static void onPauseMainMenu(void)
{
    gameState.inSplash = true;
    gameState.isPaused = false;
    playerState.gaming = gameState.isEditor;
    inputState.playerInput = false;
    bagE_setHiddenCursor(false);
    
    // COPE: + FIXME:
    camState.pitch = 0.0001f;
    camState.yaw = 0.0001f;
    camState.x = 0.0f;
    camState.y = 0.0f;
    camState.z = 0.0f;

    // FIXME:
    levelUnload(LevelBruh);
}


static GUIButtonCallback pauseButtonCallbacks[] = {
    [PauseResume]   = onPauseResume,
    [PauseSettings] = onPauseSettings,
    [PauseMainMenu] = onPauseMainMenu,
};

static_assert(length(pauseButtonCallbacks) == PauseButtonCount,
              "unfilled button callback");

static Rect pauseButtonRects[PauseButtonCount];

static PauseButtonID pauseSelectedButton = NO_BUTTON;



// TODO: remove me
static float timePassed = 0.0f;

static unsigned staticProgram;
static unsigned staticUBO;
static Matrix staticMatrixBuffer[MAX_STATIC_INSTANCE_COUNT];

static unsigned mobProgram;
static unsigned mobUBO;
static int mobBonePoolTaken;
static Matrix mobBonePool[MOB_BONE_POOL_SIZE];
static JointTransform mobTransformScratch[MAX_BONES_PER_MOB];

static bool fireDown = false;


#include "levels/bruh.c"


static void levelInits(void)
{
    for (LevelID id = 0; id < LevelCount; ++id) {
        switch (id) {
            case LevelBruh:  levelBruhInit(); break;
            case LevelCount: break;
        }
    }
}


static void levelExits(void)
{
    for (LevelID id = 0; id < LevelCount; ++id) {
        switch (id) {
            case LevelBruh:  levelBruhExit(); break;
            case LevelCount: break;
        }
    }
}


void levelLoad(LevelID id)
{
    switch (id) {
        case LevelBruh:  levelBruhLoad(); break;
        case LevelCount: break;
    }
}


void levelUnload(LevelID id)
{
    switch (id) {
        case LevelBruh:  levelBruhUnload(); break;
        case LevelCount: break;
    }

    level.filePath = NULL;

    for (MobType type = 0; type < MobCount; ++type)
        level.mobTypeCounts[type] = 0;

}


void initLevels(void)
{
    playerState = (PlayerState) {
        .gaming = false,
        .onGround = false,
        .x = 0.0f, .y = 0.0f, .z = 0.0f,
        .vy = 0.0f,
        .tryJump = true
    };


    level.skyboxProgram = createProgram(
            "shaders/cubemap_vertex.glsl",
            "shaders/cubemap_fragment.glsl"
    );

    level.terrainProgram = createProgram(
            "shaders/terrain_vertex.glsl",
            "shaders/terrain_fragment.glsl"
    );

    pointProgram = createProgram(
            "shaders/point_vertex.glsl",
            "shaders/point_fragment.glsl"
    );

    level.boxModel = createBoxModelObject();

    levelInits();


    level.textureProgram = createProgram(
            "shaders/texture_vertex.glsl",
            "shaders/texture_fragment.glsl"
    );

    level.metalProgram = createProgram(
            "shaders/metal_vertex.glsl",
            "shaders/metal_fragment.glsl"
    );

    // FIXME: at least free me
    level.gatling     = loadModelObject("res/gatling_barrel.model");
    level.gatlingBase = loadModelObject("res/gatling_base.model");
    level.glock       = loadModelObject("res/glock_top.model");
    level.glockBase   = loadModelObject("res/glock_base.model");

    level.gunTexture = createTexture("res/glock.png");


    // FIXME: at least free me
    staticUBO = createBufferObject(
        MAX_STATIC_INSTANCE_COUNT * sizeof(Matrix),
        NULL,
        GL_DYNAMIC_STORAGE_BIT
    );

    glBindBufferBase(GL_UNIFORM_BUFFER, 2, staticUBO);

    staticProgram = createProgram(
            "shaders/static_vertex.glsl",
            "shaders/static_fragment.glsl"
    );


    // FIXME: pls
    mobUBO = createBufferObject(
        sizeof(Matrix) * MOB_BONE_POOL_SIZE,
        NULL,
        GL_DYNAMIC_STORAGE_BIT
    );

    glBindBufferBase(GL_UNIFORM_BUFFER, 3, mobUBO);

    mobProgram = createProgram(
            "shaders/mob_vertex.glsl",
            "shaders/animated_fragment.glsl"
    );

    // TODO: refactor out
    Animated animated = animatedLoad("res/worm.animated");
    level.mobArmatures[MobWorm] = animated.armature;

    level.mobObjects[MobWorm] = (MobObject) {
        createAnimatedObject(animated),
        createTexture("res/worm.png")
    };

    modelFree(animated.model);
    free(animated.vertexWeights);


    level.vineThud       = loadWAV("res/vine_thud.wav",        &level.vineThudLength);
    level.bite87         = loadWAV("res/bite87.wav",           &level.bite87Length);
    level.ssAmmoPickup   = loadWAV("res/ss_ammo_pickup.wav",   &level.ssAmmoPickupLength);
    level.bruh2          = loadWAV("res/bruh2.wav",            &level.bruh2Length);
    level.gulp           = loadWAV("res/gulp.wav",             &level.gulpLength);
    level.stoneHit       = loadWAV("res/stone_hit.wav",        &level.stoneHitLength);
    level.happyWheelsWin = loadWAV("res/happy_wheels_win.wav", &level.happyWheelsWinLength);
    level.steveRev       = loadWAV("res/steve_rev.wav",        &level.steveRevLength);
    level.land           = loadWAV("res/land.wav",             &level.landLength);


    // FIXME:
    level.guiAtlas = createTexture("res/gui_atlas.png");


    level.lightProgram = createProgram(
            "shaders/texture_vertex.glsl",
            "shaders/light_fragment.glsl"
    );

    level.platform = loadModelObject("res/platform.model");
    level.platformTexture = createTexture("res/platform.png");

    // FIXME: everything in this function is not freed but it 
    //        unironically doesn't matter since the OS is unretarded
    level.head        = loadModelObject("res/head.model");
    level.headTexture = createTexture("res/stone.png");

    level.pickupObjects[HealthPickup] = (Object) {
        .model = loadModelObject("res/energy.model"),
        .texture = createTexture("res/monser.png")
    };
    level.pickupNames[HealthPickup] = "Health";

    level.pickupObjects[AmmoPickup] = (Object) {
        .model = loadModelObject("res/ammo.model"),
        .texture = createTexture("res/ammo.png")
    };
    level.pickupNames[AmmoPickup] = "Ammo";

    level.pickupObjects[HeadPickup] = (Object) {
        .model = level.head,
        .texture = level.headTexture
    };
    level.pickupNames[HeadPickup] = "Head";
}


void restartLevel(void)
{
    // FIXME: this should work for other levels as well

    levelUnload(LevelBruh);
    levelLoad(LevelBruh);
}


void exitLevels(void)
{
    // FIXME:
    if (!gameState.inSplash)
        levelUnload(LevelBruh);

    levelExits();

    freeModelObject(level.boxModel);
}


void levelsInsertStaticObject(Object object, ColliderType collider, const char *name)
{
    assert(level.statsTypeCount < MAX_STATIC_TYPE_COUNT);

    level.statsTypeObjects [level.statsTypeCount] = object;
    level.statsTypeCollider[level.statsTypeCount] = collider;
    level.statsTypeName    [level.statsTypeCount] = name;
    ++level.statsTypeCount;
}


void levelsAddStatic(int statID, ModelTransform transform)
{
    assert(level.statsInstanceCount < MAX_STATIC_INSTANCE_COUNT);

    /* O(P) pattern */
    int prevOff = level.statsTypeOffsets[++statID]++;
    ModelTransform tempTrans = level.statsTransforms[prevOff];
    level.statsTransforms[prevOff] = transform;
    transform = tempTrans;

    while (++statID <= level.statsTypeCount) {
        int off = level.statsTypeOffsets[statID]++;
        if (off != prevOff) {
            tempTrans = level.statsTransforms[off];
            level.statsTransforms[off] = transform;
            transform = tempTrans;
            prevOff = off;
        }
    }

    ++level.statsInstanceCount;

    level.recalculateStats          = true;
    level.recalculateStatsColliders = true;
}


static void removeStatic(int staticOff)
{
    /* O(P) pattern */
    int nextID = 0;
    while (level.statsTypeOffsets[nextID] <= staticOff)
        ++nextID;
    
    --nextID;

    while (++nextID <= MAX_STATIC_TYPE_COUNT) {
        int off = --level.statsTypeOffsets[nextID];
        if (off != staticOff) {
            level.statsTransforms[staticOff] = level.statsTransforms[off];
            staticOff = off;
        }
    }

    --level.statsInstanceCount;

    level.recalculateStats          = true;
    level.recalculateStatsColliders = true;
}


void staticsSave(FILE *file)
{
    safe_write("STAT", 1, 4, file);

    safe_write(&level.statsTypeCount, sizeof(int), 1, file);
    safe_write(&level.statsTypeOffsets, sizeof(int), level.statsTypeCount, file);

    safe_write(&level.statsInstanceCount, sizeof(int), 1, file);
    safe_write(&level.statsTransforms, sizeof(ModelTransform), level.statsInstanceCount, file);
}


void staticsLoad(FILE *file)
{
    char buffer[4];
    safe_read(buffer, 1, 4, file);
    if (strncmp(buffer, "STAT", 4)) {
        fprintf(stderr, "Can't parse statics file!\n");
        exit(666);
    }

    int typeCount;
    safe_read(&typeCount, sizeof(int), 1, file);
    safe_read(&level.statsTypeOffsets, sizeof(int), typeCount, file);

    safe_read(&level.statsInstanceCount, sizeof(int), 1, file);
    safe_read(&level.statsTransforms, sizeof(ModelTransform), level.statsInstanceCount, file);

    for (int i = typeCount; i <= MAX_STATIC_TYPE_COUNT; ++i)
        level.statsTypeOffsets[i] = level.statsInstanceCount;

    level.recalculateStats          = true;
    level.recalculateStatsColliders = true;
}


void spawnersSave(FILE *file)
{
    safe_write("SPWN", 1, 4, file);

    safe_write(&level.spawnerCount, sizeof(int), 1, file);
    safe_write(level.spawners, sizeof(Spawner), level.spawnerCount, file);
}


void spawnersLoad(FILE *file)
{
    char buffer[4];
    safe_read(buffer, 1, 4, file);
    if (strncmp(buffer, "SPWN", 4)) {
        fprintf(stderr, "Can't parse spawner file!\n");
        exit(666);
    }

    safe_read(&level.spawnerCount, sizeof(int), 1, file);
    safe_read(level.spawners, sizeof(Spawner), level.spawnerCount, file);

    for (int i = 0; i < level.spawnerCount; ++i)
        level.spawners[i].emitted = false;
}


void pickupsSave(FILE *file)
{
    safe_write("PICK", 1, 4, file);

    safe_write(&level.pickupCount, sizeof(int), 1, file);
    safe_write(level.pickups, sizeof(Pickup), level.pickupCount, file);
    safe_write(level.pickupPositions, sizeof(Vector), level.pickupCount, file);
}


void pickupsLoad(FILE *file)
{
    char buffer[4];
    safe_read(buffer, 1, 4, file);
    if (strncmp(buffer, "PICK", 4)) {
        fprintf(stderr, "Can't parse pickup file!\n");
        exit(666);
    }

    safe_read(&level.pickupCount, sizeof(int), 1, file);
    safe_read(level.pickups, sizeof(Pickup), level.pickupCount, file);
    safe_read(level.pickupPositions, sizeof(Vector), level.pickupCount, file);
}


static float getHeight(float x, float z)
{
    int xp = (int)x;
    int zp = (int)z;

    float rx = x - (float)xp;
    float rz = z - (float)zp;

    float height00 = atTerrainHeight(&level.terrain, xp,     zp);
    float height01 = atTerrainHeight(&level.terrain, xp,     zp + 1);
    float height10 = atTerrainHeight(&level.terrain, xp + 1, zp);
    float height11 = atTerrainHeight(&level.terrain, xp + 1, zp + 1);

    return rz          * (rx * height11 + (1.0f - rx) * height01)
         + (1.0f - rz) * (rx * height10 + (1.0f - rx) * height00);
}


void addMob(MobType type, ModelTransform trans, Animation anim)
{
    int count = level.mobTypeCounts[type];
    if (count >= MAX_MOBS_PER_TYPE) {
        fprintf(stderr, "Can't spawn another mob of type %d\n", type);
        return;
    }

    level.mobAnimations[count] = anim;
    level.mobTransforms[count] = trans;
    level.mobStates    [count] = MobStateWalking;
    level.mobAttackTOs [count] = 0.0f;
    level.mobHPs       [count] = mobStartingHPs[type];

    ++level.mobTypeCounts[type];
}


void removeMob(MobType type, int index)
{
    int pos  = type * MAX_MOBS_PER_TYPE + index;
    int last = type * MAX_MOBS_PER_TYPE + (--level.mobTypeCounts[type]);

    level.mobAnimations[pos] = level.mobAnimations[last];
    level.mobTransforms[pos] = level.mobTransforms[last];
    level.mobStates    [pos] = level.mobStates    [last];
    level.mobAttackTOs [pos] = level.mobAttackTOs [last];
    level.mobHPs       [pos] = level.mobHPs       [last];
}


void addPickup(Pickup pickup, Vector position)
{
    level.pickups        [level.pickupCount] = pickup;
    level.pickupPositions[level.pickupCount] = position;

    ++level.pickupCount;
}


void removePickup(int index)
{
    --level.pickupCount;

    level.pickups        [index] = level.pickups        [level.pickupCount];
    level.pickupPositions[index] = level.pickupPositions[level.pickupCount];
}


// TODO: replace this function by completely different one
static Vec2 clipMovement(float x, float z, float w, float vx, float vz)
{
    int cx = (int)(x / (CHUNK_TILE_DIM * CHUNK_DIM));
    int cz = (int)(z / (CHUNK_TILE_DIM * CHUNK_DIM));

    // FIXME: check neighbouring chunks as well

    int chunkPos = cz * MAX_MAP_DIM + cx;
    int colliderOffset = level.statsColliderOffsetMap[chunkPos];
    int colliderCount  = getStaticChunkColliderCount(chunkPos);

    for (int i = 0; i < colliderCount; ++i) {
        Collider collider = level.statsColliders[colliderOffset + i];
        collider.x -= x;
        collider.z -= z;

        if (fabsf(collider.x) <= (collider.sx + w) && (vz < 0.0f) == (collider.z < 0.0f)) {
            float space = fabsf(collider.z) - collider.sz - w - 0.001f;
            if (space < fabsf(vz)) {
                vz = space * (vz < 0.0f ? -1.0f : 1.0f);
            }
        }

        collider.z -= vz;

        if (fabsf(collider.z) <= (collider.sz + w) && (vx < 0.0f) == (collider.x < 0.0f)) {
            float space = fabsf(collider.x) - collider.sx - w - 0.001f;
            if (space < fabsf(vx)) {
                vx = space * (vx < 0.0f ? -1.0f : 1.0f);
            }
        }
    }

    return (Vec2) {{ vx, vz }};
}


// TODO: a magic function
void processPlayerInput(float vx, float vz, bool jump, float dt)
{
    if (playerState.hp <= 0) {
        vx = 0.0f;
        vz = 0.0f;
        jump = false;
    }

    float groundTolerance = 0.1f;
    float playerHeight = 2.0f;

    float oldY = playerState.y;

    float newX = playerState.x + vx;
    float newZ = playerState.z + vz;
    float newY = getHeight(newX, newZ);

    float distH = sqrtf(vx * vx + vz * vz);
    float distV = newY + playerHeight - oldY;

    if (distV > 0.0f) {
        float distDiff = distH - distV;
        if (distDiff <= -distH) {
            vx = 0.0f;
            vz = 0.0f;
        } else if (distDiff < 0.0f) {
            float r = 1.0f + distDiff / distH;
            vx *= r;
            vz *= r;
        }
    }

    if (playerState.onGround) {
        vx *= 0.8f;
        vz *= 0.8f;
    }

    Vec2 clipped = clipMovement(playerState.x, playerState.z, 0.5f, vx, vz);

    playerState.x += clipped.x;
    playerState.z += clipped.y;

    float height = getHeight(playerState.x, playerState.z);

    playerState.y  += playerState.vy * dt;
    playerState.vy -= 15.0f * dt;

    // TODO: remove (player height)
    height += playerHeight;
    // TODO: remove

    if (!level.inJump && (vx != 0.0f || vz != 0.0f))
        level.walkTime += dt;

    if (!level.inJump && level.walkTime > WALK_LENGTH) {
        level.walkTime -= WALK_LENGTH;

        playSound((Sound) {
            .data  = level.land,
            .end   = level.landLength,
            .volL  = 0.5f,
            .volR  = 0.5f,
            .times = 1,
        });
    }

    if (playerState.y < height + groundTolerance) {
        playerState.y = height;
        playerState.vy = 0.0f;
        playerState.onGround = true;
        level.inJump = false;
    } else {
        playerState.onGround = false;
    }

    if (jump && playerState.onGround) {
        playerState.vy += 5.0f;
        playerState.y += groundTolerance;
        level.inJump = true;
        level.walkTime = WALK_LENGTH + 0.1f;
        playSound((Sound) {
            .data  = level.steveRev,
            .end   = level.steveRevLength,
            .volL  = 0.5f,
            .volR  = 0.5f,
            .times = 1,
        });
    }
}


// TODO: REPLACE THIS CRAP
static void selectVertex(
        float camX,
        float camY,
        float camZ,
        float camPitch,
        float camYaw,
        Terrain *map
) {
    float vx = sinf(camYaw), vz = -cosf(camYaw);
    float camXS = (camX) / CHUNK_TILE_DIM + 0.5f;
    float camZS = (camZ) / CHUNK_TILE_DIM + 0.5f;
    float x = camXS;
    float z = camZS;
    float yx = -sinf(camPitch) / (cosf(camPitch));

    selected = false;

    for (int i = 0; i < 10; ++i) {
        /* NOTE: in case we are exactly aligned with an axis,
         *       we skip to avoid divide by 0 */
        if (vx == 0.0f || vz == 0.0f)
            break;

        float gx = vx > 0.0f ? ceilf(x + 0.01f) : floorf(x - 0.01f);
        float gz = vz > 0.0f ? ceilf(z + 0.01f) : floorf(z - 0.01f);

        float rx = (gx - x) / vx;
        float rz = (gz - z) / vz;

        if (rx < rz) {
            z = z + rx * vz;
            x = gx;
        } else {
            x = x + rz * vx;
            z = gz;
        }

        int xp = (int)floorf(x);
        int zp = (int)floorf(z);
        int cx = xp / CHUNK_DIM;
        int cz = zp / CHUNK_DIM;

        int chunkID = map->chunkMap[cz * MAX_MAP_DIM + cx];

        if (xp >= 0 && cx < MAX_MAP_DIM
         && zp >= 0 && cz < MAX_MAP_DIM
         && chunkID != NO_CHUNK) {
            int dx = (int)(x - camXS);
            int dz = (int)(z - camZS);
            float h = camY + sqrtf((float)(dx * dx + dz * dz)) * yx;

            int lx = xp % CHUNK_DIM;
            int lz = zp % CHUNK_DIM;

            float height = map->heights[chunkID]->data[lz * CHUNK_DIM + lx];

            if (height != NO_TILE && h < height) {
                selected = true;
                selectedX = xp;
                selectedZ = zp;
                return;
            }
        }
    }
}


void requestChunkUpdate(unsigned chunkPos)
{
    for (int i = 0; i < level.chunkUpdateCount; ++i) {
        if (level.chunkUpdates[i] == chunkPos)
            return;
    }

    level.chunkUpdates[level.chunkUpdateCount++] = chunkPos;
}


void invalidateAllChunks(void)
{
    for (int i = 0; i < MAX_MAP_DIM * MAX_MAP_DIM; ++i) {
        if (level.terrain.chunkMap[i] != NO_CHUNK)
            requestChunkUpdate(i);
    }
}


int playerRaySelect(void)
{
    float cosX =  cosf(camState.pitch);
    float sinX = -sinf(camState.pitch);
    float cosY = -cosf(camState.yaw);
    float sinY = -sinf(camState.yaw);

    float closestDist = 666.666f;
    MobType closestType;
    int closestIndex = -1;

    for (MobType type = 0; type < MobCount; ++type) {
        int count = level.mobTypeCounts[type];

        for (int i = 0; i < count; ++i) {
            ModelTransform t = level.mobTransforms[type * MAX_MOBS_PER_TYPE + i];
            float px = t.x - camState.x;
            float py = t.y - camState.y;
            float pz = t.z - camState.z;

            float x  = cosY * px + sinY * pz;
            float nz = cosY * pz - sinY * px;

            float z = cosX * nz + sinX * py;
            float y = cosX * py - sinX * nz;

            if (x < MOB_THICKNESS && x > -MOB_THICKNESS
             && y < MOB_THICKNESS && y > -MOB_THICKNESS
             && z > 0.0f) {
                if (z < closestDist) {
                    closestType = type;
                    closestIndex = i;
                    closestDist = z;
                }
            }
        }
    }

    return closestIndex == -1
         ? -1
         : (int)closestType * MAX_MOBS_PER_TYPE + closestIndex;
}


void playerShoot(int damage)
{
    int pos = playerRaySelect();

    if (pos != -1) {
        level.mobHPs[pos] -= damage;
        if (level.mobHPs[pos] <= 0)
            removeMob(pos / MAX_MOBS_PER_TYPE, pos % MAX_MOBS_PER_TYPE);
    }
}


void updateMenu(float dt)
{
    if (gameState.isEditor) {
        return;
    }

    if (pauseState == PauseSettingsState) {
        settingsUpdate(dt);
        return;
    }

    int fontSize = 8 * TEXT_MUL;
    int padding = 4 * TEXT_MUL;
    int width = fontSize * strlen(pauseButtonNames[0]);
    int height = fontSize * 2;
    int x = (appState.windowWidth - width) / 2;
    int y = (appState.windowHeight - (height + padding) * PauseButtonCount) / 2;

    for (int i = 0; i < PauseButtonCount; ++i) {
        pauseButtonRects[i] = (Rect) { x, y, width, height };
        y += height + padding;
    }
}


void menuProcessMouse(bagE_Mouse *m)
{
    int x = m->x;
    int y = m->y;

    if (gameState.isEditor && gameState.isPaused) {
        int menuHeight = EDITOR_FONT_SIZE * EditorModeCount;
        int menuWidth  = (EDITOR_FONT_SIZE / 2) * strlen(editorModeNames[0]);

        if (x < menuWidth && y < menuHeight) {
            selectedEditorMode   = y / EDITOR_FONT_SIZE;
            selectedEditorButton = EditorNoButton;
            return;
        }

        int menu2Height = EDITOR_FONT_SIZE * EditorButtonCount;
        int menu2Width  = (EDITOR_FONT_SIZE / 2) * strlen(editorButtonNames[0]);

        if (x < menu2Width && y > appState.windowHeight - menu2Height) {
            selectedEditorButton = (y - appState.windowHeight + menu2Height) / EDITOR_FONT_SIZE;
            selectedEditorMode   = NoEditorMode;
            return;
        }

        selectedEditorButton = EditorNoButton;
        selectedEditorMode   = NoEditorMode;
        return;
    }

    if (pauseState == PauseSettingsState) {
        settingsProcessMouse(m);
        return;
    }

    for (PauseButtonID id = 0; id < PauseButtonCount; ++id) {
        Rect rect = pauseButtonRects[id];

        if (x >= rect.x && x <= rect.x + rect.w
         && y >= rect.y && y <= rect.y + rect.h) {
            if (id == pauseSelectedButton)
                return;

            playSound((Sound) {
                .data = level.vineThud,
                .end   = level.vineThudLength / 16,
                .volL  = 0.15f,
                .volR  = 0.15f,
                .times = 1,
            });

            pauseSelectedButton = id;
            return;
        }
    }

    pauseSelectedButton = NO_BUTTON;
}


void updateLevel(float dt)
{
    timePassed += dt;

    selectVertex(
            camState.x, camState.y, camState.z,
            camState.pitch, camState.yaw,
            &level.terrain
    );

    /* update chunks */
    for (int i = 0; i < level.chunkUpdateCount; ++i) {
        int chunkPos = level.chunkUpdates[i];

        assert(chunkPos >= 0);

        int chunkID  = level.terrain.chunkMap[chunkPos];

        if (chunkID == NO_CHUNK)
            continue;

        updateChunkObject(
                level.terrain.objects + chunkID,
                &level.terrain,
                level.atlasViews,
                chunkPos % MAX_MAP_DIM,
                chunkPos / MAX_MAP_DIM
        );
    }

    level.chunkUpdateCount = 0;

    /* recalculate stats */
    if (level.recalculateStats) {
        level.recalculateStats = false;

        for (int i = 0; i < level.statsInstanceCount; ++i)
            staticMatrixBuffer[i] = modelTransformToMatrix(level.statsTransforms[i]);

        glNamedBufferSubData(
                staticUBO,
                0,
                sizeof(Matrix) * level.statsInstanceCount,
                staticMatrixBuffer
        );
    }

    /* recalculate static colliders */
    if (level.recalculateStatsColliders) {
        level.recalculateStatsColliders = false;

        level.statsColliderCount = 0;
        for (int i = 0; i <= MAX_MAP_DIM * MAX_MAP_DIM; ++i)
            level.statsColliderOffsetMap[i] = 0;

        for (int typeID = 0; typeID < level.statsTypeCount; ++typeID) {
            ColliderType collType = level.statsTypeCollider[typeID];

            if (!collType.inGame)
                continue;

            int offset = level.statsTypeOffsets[typeID];
            int count  = getStaticCount(typeID);

            for (int statOff = offset; statOff < offset + count; ++statOff) {
                ModelTransform trans = level.statsTransforms[statOff];

                Collider collider = collType.collider;

                collider.x *= trans.scale;
                collider.y *= trans.scale;
                collider.z *= trans.scale;

                collider.x += trans.x;
                collider.y += trans.y;
                collider.z += trans.z;

                collider.sx *= trans.scale;
                collider.sy *= trans.scale;
                collider.sz *= trans.scale;

                collider.rx += trans.rx;
                collider.ry += trans.ry;
                collider.rz += trans.rz;

                int cx = (int)(collider.x / (CHUNK_DIM * CHUNK_TILE_DIM));
                int cz = (int)(collider.z / (CHUNK_DIM * CHUNK_TILE_DIM));
                int chunkPos = cz * MAX_MAP_DIM + cx;

                /* O(P) pattern */
                int prevOff = level.statsColliderOffsetMap[++chunkPos]++;
                Collider tempColl = level.statsColliders[prevOff];
                level.statsColliders[prevOff] = collider;
                collider = tempColl;

                while (++chunkPos <= MAX_MAP_DIM * MAX_MAP_DIM) {
                    int off = level.statsColliderOffsetMap[chunkPos]++;
                    if (off != prevOff) {
                        tempColl = level.statsColliders[off];
                        level.statsColliders[off] = collider;
                        collider = tempColl;
                        prevOff = off;
                    }
                }

                ++level.statsColliderCount;
            }
        }
    }

    /* update mobs */
    for (MobType type = 0; type < MobCount; ++type) {
        int count = level.mobTypeCounts[type];

        for (int i = 0; i < count; ++i) {
            int mobID = type * MAX_MOBS_PER_TYPE + i;
            ModelTransform transform = level.mobTransforms[mobID];

            float toPlayerX = playerState.x - transform.x;
            float toPlayerZ = playerState.z - transform.z;
            float toPlayerDistance = sqrtf(toPlayerX * toPlayerX + toPlayerZ * toPlayerZ);

            level.mobAttackTOs[mobID] -= dt;
            if (level.mobAttackTOs[mobID] < 0.0f)
                level.mobAttackTOs[mobID] = 0.0f;

            if (toPlayerDistance < MOB_BITE_RANGE) {
                if (level.mobAttackTOs[mobID] == 0.0f && playerState.hp > 0) {
                    level.mobAttackTOs[mobID] = MOB_ATTACK_TO;
                    playerState.hp -= PLAYER_HP_FULL / 5;

                    if (playerState.hp < 0)
                        playerState.hp = 0;

                    playSound((Sound) {
                        .data  = level.bite87,
                        .end   = level.bite87Length,
                        .volL  = 0.5f,
                        .volR  = 0.5f,
                        .times = 1,
                    });
                }
            } else if (toPlayerDistance < MOB_CHASE_RADIUS) {
                toPlayerX /= toPlayerDistance;
                toPlayerZ /= toPlayerDistance;

                float newX = level.mobTransforms[mobID].x + toPlayerX * MOB_SPEED * dt;
                float newZ = level.mobTransforms[mobID].z + toPlayerZ * MOB_SPEED * dt;
                float newY = getHeight(newX, newZ);

                // TODO: pickups should be chunk associated and this realy
                //       should not be O(N^2) over all the mobs in the map
                bool collides = false;

                for (MobType cType = 0; cType < MobCount && !collides; ++cType) {
                    int cCount = level.mobTypeCounts[cType];

                    for (int ci = 0; ci < cCount; ++ci) {
                        int cID = cType * MAX_MOBS_PER_TYPE + ci;

                        if (cID != mobID) {
                            ModelTransform cTrans = level.mobTransforms[cID];

                            float distXS = cTrans.x - newX;
                            float distYS = cTrans.y - newY;
                            float distZS = cTrans.z - newZ;
                            distXS *= distXS;
                            distYS *= distYS;
                            distZS *= distZS;

                            if (distXS + distYS + distZS < MOB_THICKNESS * MOB_THICKNESS) {
                                collides = true;
                                break;
                            }
                        }
                    }
                }

                if (!collides) {
                    level.mobTransforms[mobID].x = newX;
                    level.mobTransforms[mobID].y = newY;
                    level.mobTransforms[mobID].z = newZ;
                }

                level.mobTransforms[mobID].ry = atanf(toPlayerX / toPlayerZ)
                                              + (toPlayerZ > 0.0f ? M_PI : 0.0f);
            }
        }
    }

    mobBonePoolTaken = 0;

    for (MobType type = 0; type < MobCount; ++type) {
        int count = level.mobTypeCounts[type];
        int boneCount = level.mobArmatures[type].boneCount;

        for (int i = 0; i < count; ++i) {
            assert(mobBonePoolTaken + boneCount < MOB_BONE_POOL_SIZE);

            int mobID = type * MAX_MOBS_PER_TYPE + i;

            updateAnimation(level.mobAnimations + mobID, dt);
            Animation anim = level.mobAnimations[mobID];
            computePoseTransforms(
                    level.mobArmatures + type,
                    mobTransformScratch,
                    anim.start + anim.time
            );

            Matrix modelMat = modelTransformToMatrix(level.mobTransforms[mobID]);
            computeArmatureMatrices(
                    modelMat,
                    mobBonePool + mobBonePoolTaken,
                    mobTransformScratch,
                    level.mobArmatures + type,
                    0
            );

            mobBonePoolTaken += boneCount;
        }
    }

    /* update guns */
    if (level.selectedGun == Glock) {
        level.gunTime += dt;
        if (level.gunTime > GLOCK_BUMP_TIME)
            level.gunTime = GLOCK_BUMP_TIME;
    } else if (level.selectedGun == Gatling) {
        float acc = (GATLING_SPIN_SPEED / GATLING_SPINUP) * dt;
        level.gatlingSpeed += fireDown ? acc : -acc;

        if (fireDown && level.gatlingSpeed > GATLING_SPIN_SPEED) {
            level.gatlingSpeed = GATLING_SPIN_SPEED;

            level.gatlingTO -= GATLING_FIRE_RATE * dt;
            if (level.gatlingTO < 0.0f) {
                level.gatlingTO = 1.0f;

                if (level.gatlingAmmo > 0) {
                    playerShoot(10);
                    playSound((Sound) {
                        .data  = level.vineThud,
                        .end   = level.vineThudLength / 8,
                        .volL  = 0.25f,
                        .volR  = 0.25f,
                        .times = 1,
                    });
                    --level.gatlingAmmo;
                }
            }

        } else if (!fireDown && level.gatlingSpeed < 0.0f) {
            level.gatlingSpeed = 0.0f;
        }

        level.gunTime += level.gatlingSpeed * dt;
    }


    /* update pickups */
    level.pickupTime += dt * PICKUP_SPEED;

    for (int i = 0; i < level.pickupCount; ) {
        Vector pos = level.pickupPositions[i];

        float distXS = playerState.x - pos.x;
        // FIXME: a hack to account for players height
        float distYS = playerState.y - pos.y - 2.0f;
        float distZS = playerState.z - pos.z;
        distXS *= distXS;
        distYS *= distYS;
        distZS *= distZS;

        float distS = distXS + distYS + distZS;

        if (distS < PICKUP_RADIUS * PICKUP_RADIUS) {
            if (pickupActions[level.pickups[i]]()) {
                removePickup(i);
                continue;
            }
        }

        ++i;
    }


    /* update platform */
    if (playerState.gaming) {
        // FIXME: the position should not be hardcoded
        float platformX = (CHUNK_DIM + CHUNK_DIM * 1.9f) * CHUNK_TILE_DIM;
        float platformZ = (CHUNK_DIM + CHUNK_DIM * 3.3f) * CHUNK_TILE_DIM;
        float platformY = getHeight(platformX, platformZ);

        float distXS = playerState.x - platformX;
        // FIXME: a hack to account for players height
        float distYS = playerState.y - platformY - 2.0f;
        float distZS = playerState.z - platformZ;
        distXS *= distXS;
        distYS *= distYS;
        distZS *= distZS;

        float distS = distXS + distYS + distZS;

        if (distS < 4.0f) {
            level.headCount += level.carryHeadCount;

            if (level.carryHeadCount > 0) {
                playSound((Sound) {
                    .data  = level.stoneHit,
                    .end   = level.stoneHitLength,
                    .volL  = 0.5f,
                    .volR  = 0.5f,
                    .times = 1,
                });
            }

            level.carryHeadCount = 0;

            if (level.headCount == 3 && !playerState.won) {
                playSound((Sound) {
                    .data  = level.happyWheelsWin,
                    .end   = level.happyWheelsWinLength,
                    .volL  = 0.5f,
                    .volR  = 0.5f,
                    .times = 1,
                });

                playerState.won = true;
            }
        }
    }


    /* editor update */
    if (terrainHeightDown && selected)
        scaleHeights((float)terrainHeightDown);
}


void renderLevel(void)
{
    /* render skybox */
    glDisable(GL_DEPTH_TEST);

    glUseProgram(level.skyboxProgram);
    glBindVertexArray(level.boxModel.vao);
    glBindTextureUnit(0, level.skyboxCubemap);

    glDrawElements(GL_TRIANGLES, BOX_INDEX_COUNT, GL_UNSIGNED_INT, 0);

    glEnable(GL_DEPTH_TEST);

    /* render terrain */
    glUseProgram(level.terrainProgram);
    glBindTextureUnit(0, level.terrainAtlas);

    for (int z = 0; z < MAX_MAP_DIM; ++z) {
        for (int x = 0; x < MAX_MAP_DIM; ++x) {
            int chunkPos = z * MAX_MAP_DIM + x;
            int chunkID;
            if ((chunkID = level.terrain.chunkMap[chunkPos]) != NO_CHUNK) {
                float xp = (float)x * CHUNK_TILE_DIM * CHUNK_DIM;
                float zp = (float)z * CHUNK_TILE_DIM * CHUNK_DIM;
                Matrix modelChunk = matrixTranslation(xp, 0.0f, zp);

                glProgramUniformMatrix4fv(level.terrainProgram, 0, 1, GL_FALSE, modelChunk.data);

                ChunkObject object = level.terrain.objects[chunkID];
                glBindVertexArray(object.vao);
                glDrawElements(GL_TRIANGLES, object.indexCount, GL_UNSIGNED_INT, 0);
            }
        }
    }

    Matrix mul;

    /* render gun */
    if (playerState.gaming) {
        float scale = level.selectedGun == Gatling ? 0.5f : 0.2f;
        Matrix modelGun = matrixScale(scale, scale, scale);

        if (level.selectedGun == Gatling) {
            mul = matrixRotationZ(level.gunTime * M_PI * 2);
            modelGun = matrixMultiply(&mul, &modelGun);
        } else {
            mul = matrixRotationX((M_PI / 3) * sinf((level.gunTime / GLOCK_BUMP_TIME) * M_PI));
            modelGun = matrixMultiply(&mul, &modelGun);

            mul = matrixRotationY(M_PI);
            modelGun = matrixMultiply(&mul, &modelGun);
        }

        float offset = level.selectedGun == Gatling
                     ? 0.5f
                     : 0.75f -  0.2f * sinf((level.gunTime / GLOCK_BUMP_TIME) * M_PI);

        mul = matrixTranslation(-0.5f, -0.5f, offset);
        modelGun = matrixMultiply(&mul, &modelGun);

        mul = matrixRotationX(camState.pitch);
        modelGun = matrixMultiply(&mul, &modelGun);

        mul = matrixRotationY(-camState.yaw + M_PI);
        modelGun = matrixMultiply(&mul, &modelGun);

        mul = matrixTranslation(camState.x, camState.y, camState.z);
        modelGun = matrixMultiply(&mul, &modelGun);

        glUseProgram(level.metalProgram);
        glProgramUniformMatrix4fv(level.metalProgram, 0, 1, GL_FALSE, modelGun.data);

        if (level.selectedGun == Gatling) {
            glBindVertexArray(level.gatling.vao);
            glDrawElements(GL_TRIANGLES, level.gatling.indexCount, GL_UNSIGNED_INT, 0);
        } else {
            glBindVertexArray(level.glock.vao);
            glDrawElements(GL_TRIANGLES, level.glock.indexCount, GL_UNSIGNED_INT, 0);

            glUseProgram(level.textureProgram);
            glProgramUniformMatrix4fv(level.textureProgram, 0, 1, GL_FALSE, modelGun.data);

            glBindVertexArray(level.glockBase.vao);
            glBindTextureUnit(0, level.gunTexture);
            glDrawElements(GL_TRIANGLES, level.glockBase.indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    /* render statics */
    glUseProgram(staticProgram);

    for (int i = 0; i < level.statsTypeCount; ++i) {
        Object object = level.statsTypeObjects[i];
        glBindVertexArray(object.model.vao);
        glBindTextureUnit(0, object.texture);

        glProgramUniform1ui(staticProgram, 0, level.statsTypeOffsets[i]);

        glDrawElementsInstanced(
                GL_TRIANGLES,
                object.model.indexCount,
                GL_UNSIGNED_INT,
                0,
                getStaticCount(i)
        );
    }

    /* render mobs */
    glNamedBufferSubData(
            mobUBO,
            0,
            sizeof(Matrix) * mobBonePoolTaken,
            (float*)mobBonePool
    );

    glUseProgram(mobProgram);

    int mobOffset = 0;
    for (MobType type = 0; type < MobCount; ++type) {
        MobObject object = level.mobObjects[type];
        glBindVertexArray(object.animated.model.vao);
        glBindTextureUnit(0, object.texture);

        glProgramUniform1ui(mobProgram, 0, mobOffset);
        glProgramUniform1ui(mobProgram, 1, level.mobArmatures[type].boneCount);

        glDrawElementsInstanced(
                GL_TRIANGLES,
                object.animated.model.indexCount,
                GL_UNSIGNED_INT,
                0,
                level.mobTypeCounts[type]
        );

        mobOffset += level.mobTypeCounts[type] * level.mobArmatures[type].boneCount;
    }


    /* render pickups */
    glUseProgram(level.textureProgram);

    for (Pickup pickup = 0; pickup < PickupCount; ++pickup) {
        Object object = level.pickupObjects[pickup];
        glBindVertexArray(object.model.vao);
        glBindTextureUnit(0, object.texture);

        for (int i = 0; i < level.pickupCount; ++i) {
            if (level.pickups[i] == pickup) {
                Vector pos = level.pickupPositions[i];

                float scale = pickupScales[pickup];
                Matrix modelMat = matrixScale(scale, scale, scale);

                mul = matrixRotationY(level.pickupTime);
                modelMat = matrixMultiply(&mul, &modelMat);

                mul = matrixTranslation(pos.x, pos.y + 0.25f, pos.z);
                modelMat = matrixMultiply(&mul, &modelMat);

                glProgramUniformMatrix4fv(level.textureProgram, 0, 1, GL_FALSE, modelMat.data);
                glDrawElements(GL_TRIANGLES, object.model.indexCount, GL_UNSIGNED_INT, 0);
            }
        }
    }


    // FIXME: render platform
    //        hardcoded for now but should not be here
    glUseProgram(level.lightProgram);

    glBindVertexArray(level.platform.vao);
    glBindTextureUnit(0, level.platformTexture);

    float platformX = (CHUNK_DIM + CHUNK_DIM * 1.9f) * CHUNK_TILE_DIM;
    float platformZ = (CHUNK_DIM + CHUNK_DIM * 3.3f) * CHUNK_TILE_DIM;
    float platformY = getHeight(platformX, platformZ);

    Matrix platformMat = matrixScale(2.5f, 2.5f, 2.5f);

    mul = matrixTranslation(platformX, platformY, platformZ);
    platformMat = matrixMultiply(&mul, &platformMat);

    glProgramUniformMatrix4fv(level.lightProgram, 0, 1, GL_FALSE, platformMat.data);
    glProgramUniform4f(level.lightProgram, 1, platformX, platformY + 1.0f, platformZ, 0.0f);
    glProgramUniform4f(level.lightProgram, 2, 1.0f, 0.0f, 0.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, level.platform.indexCount, GL_UNSIGNED_INT, 0);

    if (level.headCount >= 1) {
        glBindVertexArray(level.head.vao);
        glBindTextureUnit(0, level.headTexture);

        Matrix headMat = matrixRotationY(level.pickupTime * PICKUP_SPEED);

        mul = matrixTranslation(platformX - 2.0f, platformY + 1.5f, platformZ);
        headMat = matrixMultiply(&mul, &headMat);

        glProgramUniformMatrix4fv(level.lightProgram, 0, 1, GL_FALSE, headMat.data);
        glDrawElements(GL_TRIANGLES, level.head.indexCount, GL_UNSIGNED_INT, 0);
    }

    if (level.headCount >= 2) {
        Matrix headMat = matrixRotationY(level.pickupTime * PICKUP_SPEED);

        mul = matrixTranslation(platformX + 2.0f, platformY + 1.5f, platformZ);
        headMat = matrixMultiply(&mul, &headMat);

        glProgramUniformMatrix4fv(level.lightProgram, 0, 1, GL_FALSE, headMat.data);
        glDrawElements(GL_TRIANGLES, level.head.indexCount, GL_UNSIGNED_INT, 0);
    }

    if (level.headCount >= 3) {
        Matrix headMat = matrixRotationY(level.pickupTime * PICKUP_SPEED);

        mul = matrixTranslation(platformX, platformY + 1.5f, platformZ + 2.0f);
        headMat = matrixMultiply(&mul, &headMat);

        glProgramUniformMatrix4fv(level.lightProgram, 0, 1, GL_FALSE, headMat.data);
        glDrawElements(GL_TRIANGLES, level.head.indexCount, GL_UNSIGNED_INT, 0);
    }


    /* editor render mob spawners */
    if (!playerState.gaming && editorMode == SpawnerPlacing) {
        glUseProgram(level.textureProgram);

        for (MobType type = 0; type < MobCount; ++type) {
            MobObject object = level.mobObjects[type];
            glBindVertexArray(object.animated.model.vao);
            glBindTextureUnit(0, object.texture);

            for (int i = 0; i < level.spawnerCount; ++i) {
                Spawner s = level.spawners[i];
                if (s.type != type || s.emitted)
                    break;

                // FIXME: This rendering is busted but it looks cool so
                //        I don't care for now
                Matrix modelMat = matrixTranslation(s.x, s.y, s.z);
                glProgramUniformMatrix4fv(level.textureProgram, 0, 1, GL_FALSE, modelMat.data);
                glDrawElements(GL_TRIANGLES, object.animated.model.indexCount, GL_UNSIGNED_INT, 0);
            }
        }
    }

    /* editor render static colliders */
    if (!playerState.gaming && editorMode == StaticsPlacing) {
        glUseProgram(pointProgram);

        Vector colors2[] = {
            {{ 0.3f, 1.0f, 0.6f, 1.0f }}, {{ 0.3f, 1.0f, 0.6f, 1.0f }},
            {{ 0.3f, 1.0f, 0.6f, 1.0f }}, {{ 0.3f, 1.0f, 0.6f, 1.0f }},
            {{ 0.3f, 1.0f, 0.6f, 1.0f }}, {{ 0.3f, 1.0f, 0.6f, 1.0f }},
            {{ 0.3f, 1.0f, 0.6f, 1.0f }}, {{ 0.3f, 1.0f, 0.6f, 1.0f }},
        };

        Vector points2[] = {
            {{ -1.0f, -1.0f, -1.0f, 12.0f }}, {{  1.0f, -1.0f, -1.0f, 12.0f }},
            {{ -1.0f, -1.0f,  1.0f, 12.0f }}, {{  1.0f, -1.0f,  1.0f, 12.0f }},
            {{ -1.0f,  1.0f, -1.0f, 12.0f }}, {{  1.0f,  1.0f, -1.0f, 12.0f }},
            {{ -1.0f,  1.0f,  1.0f, 12.0f }}, {{  1.0f,  1.0f,  1.0f, 12.0f }},
        };

        glProgramUniform4fv(pointProgram, 1,  8, colors2->data);
        glProgramUniform4fv(pointProgram, 65, 8, points2->data);

        for (int i = 0; i < level.statsColliderCount; ++i) {
            Collider c = level.statsColliders[i];

            Matrix modMat = matrixScale(c.sx, c.sy, c.sz);
            mul = matrixTranslation(c.x, c.y, c.z);
            modMat = matrixMultiply(&mul, &modMat);

            glProgramUniformMatrix4fv(pointProgram, 0, 1, false, modMat.data);
            glDrawArraysInstanced(GL_POINTS, 0, 1, 8);
        }
    }
}


Matrix modelTransformToMatrix(ModelTransform transform)
{
    Matrix res = matrixScale(transform.scale, transform.scale, transform.scale);

    Matrix mul = matrixRotationZ(transform.rz);
    res = matrixMultiply(&mul, &res);

    mul = matrixRotationY(transform.ry);
    res = matrixMultiply(&mul, &res);

    mul = matrixRotationX(transform.rx);
    res = matrixMultiply(&mul, &res);

    mul = matrixTranslation(transform.x, transform.y, transform.z);
    res = matrixMultiply(&mul, &res);

    return res;
}


static void updateNearbyChunks(int x, int z)
{
    int cx = x / CHUNK_DIM;
    int rx = x % CHUNK_DIM;
    int cz = z / CHUNK_DIM;
    int rz = z % CHUNK_DIM;

    // FIXME: check for MAX_MAP_DIM
    for (int zo = -1; zo <= 1; ++zo) {
        for (int xo = -1; xo <= 1; ++xo) {
            if ((xo == 0 || (((rx + (xo > 0))) % CHUNK_DIM == 0 && x != 0))
             && (zo == 0 || (((rz + (zo > 0))) % CHUNK_DIM == 0 && z != 0))) {
                requestChunkUpdate((cz + zo) * MAX_MAP_DIM + (cx + xo));
            }
        }
    }
}


static void extendHeights(void)
{
    float midHeight = atTerrainHeight(&level.terrain, selectedX, selectedZ);

    for (int z = -brushWidth; z <= brushWidth; ++z) {
        for (int x = -brushWidth; x <= brushWidth; ++x) {
            int xp = selectedX + x;
            int zp = selectedZ + z;

            float height = atTerrainHeight(&level.terrain, xp, zp);

            // FIXME: check for MAX_MAP_DIM
            if (height == NO_TILE && xp >= 0 && zp >= 0) {
                setTerrainHeight(&level.terrain, xp, zp, midHeight);
                updateNearbyChunks(xp, zp);
            }
        }
    }
}


static void scaleHeights(float scale)
{
    for (int z = -brushWidth; z <= brushWidth; ++z) {
        for (int x = -brushWidth; x <= brushWidth; ++x) {
            int xp = selectedX + x;
            int zp = selectedZ + z;

            float height = atTerrainHeight(&level.terrain, xp, zp);

            // FIXME: check for MAX_MAP_DIM
            if (height != NO_TILE && xp >= 0 && zp >= 0) {
                float invDist = 1.0f;
                if (brushWidth > 0) {
                    invDist = 1.0f - (sqrtf((float)(x * x + z * z))
                                    / sqrtf((float)(brushWidth * brushWidth * 2)));
                }

                float newHeight = height + selectedRelHeight * scale * invDist;
                setTerrainHeight(&level.terrain, xp, zp, newHeight);
                updateNearbyChunks(xp, zp);
            }
        }
    }
}


static void removeTiles(void)
{
    for (int z = -brushWidth; z <= brushWidth; ++z) {
        for (int x = -brushWidth; x <= brushWidth; ++x) {
            int xp = selectedX + x;
            int zp = selectedZ + z;

            // FIXME: check for MAX_MAP_DIM
            if (xp >= 0 && zp >= 0) {
                setTerrainHeight(&level.terrain, xp, zp, NO_TILE);
                updateNearbyChunks(xp, zp);
            }
        }
    }
}


void addSpawner(Spawner spawner)
{
    level.spawners[level.spawnerCount++] = spawner;
}


void removeSpawner(int index)
{
    level.spawners[index] = level.spawners[--level.spawnerCount];
}


void spawnersBroadcast(SpawnerGroup group)
{
    for (int i = 0; i < level.spawnerCount; ++i) {
        Spawner s = level.spawners[i];

        if (s.emitted || s.group != group)
            continue;

        // FIXME: this currently works for worms only
        float animLength = level.mobArmatures[MobWorm].timeStamps[2];

        addMob(MobWorm,
                (ModelTransform) { s.x, s.y, s.z, 2.0f },
                (Animation) { .start = level.mobArmatures[MobWorm].timeStamps[0],
                              .end   = animLength,
                              .time  = (animLength / 10) * (rand() % 10) });

        level.spawners[i].emitted = true;
    }
}


void levelsProcessButton(bagE_MouseButton *mb, bool down)
{
    if (!gameState.isEditor) {
        if (gameState.isPaused) {
            if (pauseState == PauseSettingsState) {
                if (settingsClick(mb, down))
                    pauseState = PauseBaseState;

                return;
            }

            if (!down || pauseSelectedButton == NO_BUTTON)
                return;

            pauseButtonCallbacks[pauseSelectedButton]();
            return;
        }

        if (playerState.hp <= 0)
            return;

        if (level.selectedGun == Glock) {
            if (!down)
                return;

            if (mb->button == bagE_ButtonLeft && level.gunTime == GLOCK_BUMP_TIME) {
                level.gunTime = 0.0f;
                playerShoot(20);
                playSound((Sound) {
                    .data = level.vineThud,
                    .end   = level.vineThudLength,
                    .volL  = 0.25f,
                    .volR  = 0.25f,
                    .times = 1,
                });
            }
        } else if (level.selectedGun == Gatling) {
            if (mb->button == bagE_ButtonLeft)
                fireDown = down;
        }
    } else {
        if (gameState.isPaused) {
            if (!down)
                return;

            if (selectedEditorMode != NoEditorMode) {
                editorMode = selectedEditorMode;
            } else if (selectedEditorButton != EditorNoButton) {
                switch (selectedEditorButton) {
                    case EditorMainMenu:
                        onPauseMainMenu();
                        break;

                    case EditorSaveLevel:
                        levelsSaveCurrent();
                        break;

                    case EditorButtonCount:
                        unreachable();
                }
            }

            return;
        }

        switch (editorMode) {
            case TerrainPlacing:
                if (!down)
                    return;

                if (mb->button == bagE_ButtonLeft) {
                    extendHeights();
                } else if (mb->button == bagE_ButtonRight) {
                    removeTiles();
                }
                break;
            // case TerrainHeightPaintingAbs:
                // /* fallthrough */
            case TerrainHeightPaintingRel: {
                    int dir = 0;
                    if (mb->button == bagE_ButtonLeft) {
                        dir = 1;
                    } else if (mb->button == bagE_ButtonRight) {
                        dir = -1;
                    }
                    terrainHeightDown = dir * down;
                }
                break;
            case TerrainTexturing:
                if (!down)
                    return;

                if (mb->button == bagE_ButtonLeft && selected) {
                    TileTexture tileTex = {
                        .viewID = selectedViewID,
                    };
                    setTerrainTexture(&level.terrain, selectedX, selectedZ, tileTex);
                    updateNearbyChunks(selectedX, selectedZ);
                }
                break;
            case StaticsPlacing:
                if (!down)
                    return;

                if (mb->button == bagE_ButtonLeft && selected) {
                    levelsAddStatic(selectedStaticID, (ModelTransform) {
                        .x = selectedX * CHUNK_TILE_DIM,
                        .y = atTerrainHeight(&level.terrain, selectedX, selectedZ),
                        .z = selectedZ * CHUNK_TILE_DIM,
                        .scale = 1.0f,
                        .ry = ((M_PI * 2) / 100) * (rand() % 100), // FIXME: temporary
                    });
                } else if (mb->button == bagE_ButtonRight) {
                    float x = selectedX * CHUNK_TILE_DIM;
                    float y = atTerrainHeight(&level.terrain, selectedX, selectedZ);
                    float z = selectedZ * CHUNK_TILE_DIM;

                    // TODO: could be done in one pass
                    int index = -1;
                    float distS = INFINITY;

                    for (int i = 0; i < level.statsInstanceCount; ++i) {
                        ModelTransform transform = level.statsTransforms[i];
                        float xd  = x - transform.x;
                        float yd  = y - transform.y;
                        float zd  = z - transform.z;
                        float newDistS = xd * xd + yd * yd + zd * zd;

                        if (newDistS < distS) {
                            distS = newDistS;
                            index = i;
                        }
                    }

                    if (index >= 0 && distS < CHUNK_TILE_DIM * CHUNK_TILE_DIM * 1.5f * 1.5f)
                        removeStatic(index);
                }
                break;
            case SpawnerPlacing:
                if (!down)
                    return;

                if (selected) {
                    float x = selectedX * CHUNK_TILE_DIM;
                    float z = selectedZ * CHUNK_TILE_DIM;
                    float y = getHeight(x, z);

                    if (mb->button == bagE_ButtonLeft) {
                        addSpawner((Spawner) { selectedSpawnerType, SpawnerInit, x, y + 1.0f, z });
                    } else {
                        float minDist = 666.666f;
                        int closest = -1;

                        for (int i = 0; i < level.spawnerCount; ++i) {
                            Spawner s = level.spawners[i];
                            float dx = x - s.x;
                            float dy = y - s.y;
                            float dz = z - s.z;
                            float dist =  dx * dx + dy * dy + dz * dz;

                            if (dist < minDist) {
                                minDist = dist;
                                closest = i;
                            }
                        }

                        if (closest != -1)
                            removeSpawner(closest);
                    }
                }
                break;
            case PickupPlacing:
                if (!down)
                    return;

                if (selected) {
                    float x = selectedX * CHUNK_TILE_DIM;
                    float z = selectedZ * CHUNK_TILE_DIM;
                    float y = getHeight(x, z);

                    if (mb->button == bagE_ButtonLeft) {
                        addPickup(selectedPickup, (Vector) {{ x, y, z }});
                    } else {
                        float minDist = 666.666f;
                        int closest = -1;

                        for (int i = 0; i < level.pickupCount; ++i) {
                            Vector s = level.pickupPositions[i];
                            float dx = x - s.x;
                            float dy = y - s.y;
                            float dz = z - s.z;
                            float dist =  dx * dx + dy * dy + dz * dz;

                            if (dist < minDist) {
                                minDist = dist;
                                closest = i;
                            }
                        }

                        if (closest != -1)
                            removePickup(closest);
                    }
                }
                break;

            case EditorModeCount:
                unreachable();
        }
    }
}


void levelsProcessWheel(bagE_MouseWheel *mw)
{
    if (!gameState.isEditor) {
        if (playerState.hp <= 0)
            return;

        level.selectedGun = (level.selectedGun + 1) % GunTypeCount;
    } else {
        switch (editorMode) {
            case TerrainPlacing:
                /* fallthrough */
            // case TerrainHeightPaintingAbs:
                // /* fallthrough */

            case TerrainHeightPaintingRel:
                brushWidth += mw->scrollUp;
                if (brushWidth < 0)
                    brushWidth = 0;
                if (brushWidth > MAX_BRUSH_WIDTH)
                    brushWidth = MAX_BRUSH_WIDTH;
                break;

            case StaticsPlacing:
                selectedStaticID += mw->scrollUp;
                if (selectedStaticID < 0)
                    selectedStaticID = 0;
                if (selectedStaticID >= level.statsTypeCount)
                    selectedStaticID = level.statsTypeCount - 1;
                break;

            case PickupPlacing:
                selectedPickup += mw->scrollUp;
                if (selectedPickup < 0)
                    selectedPickup = 0;
                if (selectedPickup >= PickupCount)
                    selectedPickup = PickupCount - 1;
                break;
            
            case TerrainTexturing:
                selectedViewID += mw->scrollUp;
                if (selectedViewID == 255)
                    selectedViewID = 0;
                if (selectedViewID >= level.atlasViewCount)
                    selectedViewID = level.atlasViewCount - 1;
                break;

            case SpawnerPlacing:
                break;

            case EditorModeCount:
                unreachable();
        }
    }
}


void levelsSaveCurrent(void)
{
    assert(level.filePath != NULL);

    FILE *file = fopen(level.filePath, "wb");
    file_check(file, level.filePath);

    terrainSave(&level.terrain, file);
    staticsSave(file);
    spawnersSave(file);
    pickupsSave(file);

    fclose(file);

    printf("saved \"%s\".\n", level.filePath);
}


void processEsc(void)
{
    if (pauseState != PauseBaseState) {
        if (settingsProcessEsc())
            pauseState = PauseBaseState;
        return;
    }

    inputState.playerInput = !inputState.playerInput;
    gameState.isPaused = !gameState.isPaused;
    bagE_setHiddenCursor(inputState.playerInput);
}


void renderLevelOverlay(void)
{
    if (!gameState.isEditor) {
        Color healthColor = {{ 1.0f, 0.0f, 0.0f, 1.0f }};
        Color backColor   = {{ 0.2f, 0.0f, 0.0f, 1.0f }};
        Color ammoBack    = {{ 0.2f, 0.2f, 0.2f, 1.0f }};
        Color textColor   = {{ 1.0f, 1.0f, 1.0f, 1.0f }};

        int barWidth  = appState.windowWidth / 5;
        int barHeight = barWidth / 10;
        int margin    = barHeight * 2;
        int padding   = 3;

        int healthSize = barHeight * 4;

        int ammoSize     = barHeight * 4;
        int ammoTextSize = 16;

        guiBeginRect();
        guiDrawRect(
                margin + healthSize,
                appState.windowHeight - margin - barHeight,
                barWidth, barHeight,
                backColor
        );
        guiDrawRect(
                margin + healthSize + padding,
                appState.windowHeight - margin - barHeight + padding,
                (int)(((float)barWidth / PLAYER_HP_FULL) * playerState.hp) - 2 * padding,
                barHeight - 2 * padding,
                healthColor
        );
        guiDrawRect(
                appState.windowWidth  - margin * 2 - ammoSize * 2,
                appState.windowHeight - margin / 2 - ammoTextSize * 2,
                ammoSize, ammoTextSize * 2,
                ammoBack
        );
        guiDrawRect(
                appState.windowWidth  - margin - ammoSize,
                appState.windowHeight - margin / 2 - ammoTextSize * 2,
                ammoSize, ammoTextSize * 2,
                ammoBack
        );

        int deathThickness = 300;

        if (playerState.won) {
            guiDrawRect(
                    0,
                    (appState.windowHeight - deathThickness) / 2,
                    appState.windowWidth, deathThickness,
                    (Color) {{ 1.0f, 1.0f, 1.0f, 1.0f }}
            );
        } else if (playerState.hp <= 0) {
            guiDrawRect(
                    0,
                    (appState.windowHeight - deathThickness) / 2,
                    appState.windowWidth, deathThickness,
                    (Color) {{ 0.0f, 0.0f, 0.0f, 1.0f }}
            );
        }

        guiBeginImage();
        guiUseImage(level.guiAtlas);
        guiDrawImage(
                margin / 2,
                appState.windowHeight - healthSize,
                healthSize, healthSize,
                0.0f, 0.0f, 0.25f, 0.25f
        );
        guiDrawImage(
                appState.windowWidth  - margin * 2 - ammoSize * 2,
                appState.windowHeight - margin / 2 - ammoTextSize * 2 - ammoSize,
                ammoSize, ammoSize,
                0.5f, 0.0f, 0.25f, 0.25f
        );
        guiDrawImage(
                appState.windowWidth  - margin - ammoSize,
                appState.windowHeight - margin / 2 - ammoTextSize * 2 - ammoSize,
                ammoSize, ammoSize,
                0.75f, 0.0f, 0.25f, 0.25f
        );
        guiDrawImageColored(
                appState.windowWidth - margin - ammoSize - ammoTextSize * 2,
                margin / 2,
                ammoSize, ammoSize,
                0.0f, 0.25f, 0.25f, 0.25f,
                (level.carryHeadCount == 0 ? (Color) {{ 0.0f, 0.0f, 0.0f, 1.0f }}
                                           : (Color) {{ 1.0f, 1.0f, 1.0f, 1.0f }})
        );

        int crossSize = 32;
        Color crossColor = {{ 1.0f, 1.0f, 1.0f, 1.0f }};

        int mobPos = playerRaySelect();
        if (mobPos != -1) {
            MobType type = mobPos / MAX_MOBS_PER_TYPE;
            int part = mobStartingHPs[type] / 4;

            int hp = level.mobHPs[mobPos];
            if (hp <= part) {
                crossColor = (Color) {{ 1.0f, 0.0f, 0.0f, 1.0f }};
            } else if (hp <= part * 2) {
                crossColor = (Color) {{ 1.0f, 0.5f, 0.0f, 1.0f }};
            } else if (hp <= part * 3) {
                crossColor = (Color) {{ 1.0f, 1.0f, 0.0f, 1.0f }};
            } else {
                crossColor = (Color) {{ 0.0f, 1.0f, 0.0f, 1.0f }};
            }
        }

        guiDrawImageColored(
                (appState.windowWidth  - crossSize) / 2,
                (appState.windowHeight - crossSize) / 2,
                crossSize, crossSize,
                0.25f, 0.0f, 0.25f, 0.25f,
                crossColor
        );

        guiBeginText();
        guiDrawText("INF",
                appState.windowWidth  - margin * 2 - ammoSize - (ammoSize + 3 * ammoTextSize) / 2,
                appState.windowHeight - margin / 2 - ammoTextSize * 2,
                ammoTextSize, ammoTextSize * 2, 0,
                textColor
        );

        char buffer[64] = { 0 };
        int written = snprintf(buffer, 64, "%d", level.gatlingAmmo);
        guiDrawText(buffer,
                appState.windowWidth  - margin - (ammoSize + written * ammoTextSize) / 2,
                appState.windowHeight - margin / 2 - ammoTextSize * 2,
                ammoTextSize, ammoTextSize * 2, 0,
                textColor
        );

        written = snprintf(buffer, 64, "x%d", level.carryHeadCount);
        guiDrawText(buffer,
                appState.windowWidth - margin - ammoTextSize * 2,
                margin,
                ammoTextSize, ammoTextSize * 2, 0,
                textColor
        );

        if (playerState.won) {
            guiDrawText("YOU WIN!",
                    (appState.windowWidth - 8 * 160) / 2,
                    (appState.windowHeight - 320) / 2,
                    160, 320, 0,
                    (Color) {{ 0.0f, 0.0f, 0.0f, 1.0f }}
            );
        } else if (playerState.hp <= 0) {
            guiDrawText("YOU DIED",
                    (appState.windowWidth - 8 * 160) / 2,
                    (appState.windowHeight - 320) / 2,
                    160, 320, 0,
                    (Color) {{ 1.0f, 0.0f, 0.0f, 1.0f }}
            );

            const char *hint = "Press \"space\" to restart level";
            int hintSize = 32;
            int hintLen  = strlen(hint);
            guiDrawText(hint,
                    (appState.windowWidth - hintSize * hintLen) / 2,
                    (appState.windowHeight + 400) / 2,
                    hintSize, hintSize * 2, 0,
                    (Color) {{ 0.0f, 0.0f, 0.0f, 1.0f }}
            );
        }

        /* pause menu */
        if (gameState.isPaused) {
            Color screenTint = {{ 0.2f, 0.3f, 0.4f, 0.75f }};
            Color textColor  = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
            Color textColor2 = {{ 1.0f, 1.0f, 1.0f, 1.0f }};

            guiBeginRect();
            guiDrawRect(0, 0, appState.windowWidth, appState.windowHeight, screenTint);

            if (pauseState == PauseSettingsState) {
                settingsRender();
                return;
            }

            guiBeginText();
            for (PauseButtonID id = 0; id < PauseButtonCount; ++id) {
                Rect rect = pauseButtonRects[id];
                if (id == pauseSelectedButton) {
                    guiDrawText(pauseButtonNames[id],
                                rect.x, rect.y, rect.h / 2, rect.h, 0,
                                textColor2);
                } else {
                    guiDrawText(pauseButtonNames[id],
                                rect.x, rect.y, rect.h / 2, rect.h, 0,
                                textColor);
                }
            }
        }
    } else {
        /* point */
        if (selected) {
            glUseProgram(pointProgram);

            int pointCount = 0;

            Vector colors[64];
            Vector points[64];

            float midHeight = atTerrainHeight(&level.terrain, selectedX, selectedZ);

            int fromZ, toZ, fromX, toX;

            if (editorMode == TerrainTexturing) {
                AtlasView view = level.atlasViews[selectedViewID];
                int width  = view.wn;
                int height = view.hn;

                fromX = -(width  / 2);
                toX   =   width  / 2 + width  % 2;
                fromZ = -(height / 2);
                toZ   =   height / 2 + height % 2;
            } else if (editorMode == TerrainPlacing
                    || editorMode == TerrainHeightPaintingRel) {
                    // || editorMode == TerrainHeightPaintingAbs) {
                fromZ = -brushWidth;
                toZ   =  brushWidth;
                fromX = -brushWidth;
                toX   =  brushWidth;
            } else {
                fromZ = 0;
                toZ   = 0;
                fromX = 0;
                toX   = 0;
            }

            for (int z = fromZ; z <= toZ; ++z) {
                for (int x = fromX; x <= toX; ++x) {
                    float height = atTerrainHeight(&level.terrain, selectedX + x, selectedZ + z);

                    if (height == NO_TILE) {
                        height = midHeight;
                        colors[pointCount] = (Vector) { { 0.5f, 1.0f,  0.75f, 1.0f } };
                    } else if (x == 0 && z == 0) {
                        colors[pointCount] = (Vector) { { 1.0f, 0.5f,  0.75f, 1.0f } };
                    } else {
                        colors[pointCount] = (Vector) { { 1.0f, 0.75f, 0.75f, 1.0f } };
                    }

                    points[pointCount] = (Vector) { {
                        (selectedX + x) * CHUNK_TILE_DIM,
                        height,
                        (selectedZ + z) * CHUNK_TILE_DIM,
                        (x == 0 && z == 0 ? 18.0f : 12.0f)
                    } };

                    ++pointCount;
                }
            }

            Matrix identity = matrixIdentity();
            glProgramUniformMatrix4fv(pointProgram, 0, 1, false, identity.data);
            glProgramUniform4fv(pointProgram, 1,  pointCount, colors->data);
            glProgramUniform4fv(pointProgram, 65, pointCount, points->data);

            glDrawArraysInstanced(GL_POINTS, 0, 1, pointCount);
        }

        Color screenTint = {{ 0.2f, 0.3f, 0.4f, 0.75f }};
        Color textColor  = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
        Color textColor2 = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

        /* render editor menu */
        if (gameState.isPaused) {
            guiBeginRect();
            guiDrawRect(0, 0, 300, appState.windowHeight, screenTint);
            guiDrawRect(0, editorMode * EDITOR_FONT_SIZE,
                        strlen(editorModeNames[0]) * EDITOR_FONT_SIZE / 2, EDITOR_FONT_SIZE,
                        textColor);

            guiBeginText();

            for (EditorMode mode = 0; mode < EditorModeCount; ++mode) {
                Color color = (mode == selectedEditorMode && mode != editorMode)
                            ? textColor : textColor2;

                guiDrawText(editorModeNames[mode], 0, mode * EDITOR_FONT_SIZE,
                            EDITOR_FONT_SIZE / 2, EDITOR_FONT_SIZE, 0, color);
            }

            for (EditorButtonID id = 0; id < EditorButtonCount; ++id) {
                Color color = id == selectedEditorButton ? textColor : textColor2;

                int offset = appState.windowHeight - EDITOR_FONT_SIZE * EditorButtonCount;

                guiDrawText(editorButtonNames[id], 0, offset + id * EDITOR_FONT_SIZE,
                            EDITOR_FONT_SIZE / 2, EDITOR_FONT_SIZE, 0, color);
            }
        }

        char charBuffer[64];

        /* render editor overlay */
        switch (editorMode) {
            case TerrainPlacing:
                /* fallthrough */
            case TerrainHeightPaintingRel:
                guiBeginText();
                snprintf(charBuffer, 64, "brush width: %d", brushWidth + 1);
                guiDrawText(charBuffer,
                            appState.windowWidth - strlen(charBuffer) * (EDITOR_FONT_SIZE / 2),
                            0, EDITOR_FONT_SIZE / 2, EDITOR_FONT_SIZE, 0, textColor);
                break;

            case StaticsPlacing:
                guiBeginText();
                snprintf(charBuffer, 64, "type: %s", level.statsTypeName[selectedStaticID]);
                guiDrawText(charBuffer,
                            appState.windowWidth - strlen(charBuffer) * (EDITOR_FONT_SIZE / 2),
                            0, EDITOR_FONT_SIZE / 2, EDITOR_FONT_SIZE, 0, textColor);
                break;

            case PickupPlacing:
                guiBeginText();
                snprintf(charBuffer, 64, "type: %s", level.pickupNames[selectedPickup]);
                guiDrawText(charBuffer,
                            appState.windowWidth - strlen(charBuffer) * (EDITOR_FONT_SIZE / 2),
                            0, EDITOR_FONT_SIZE / 2, EDITOR_FONT_SIZE, 0, textColor);
                break;

            case TerrainTexturing:
                guiBeginImage();
                guiUseImage(level.terrainAtlas);
                AtlasView view = level.atlasViews[selectedViewID];
                guiDrawImage(appState.windowWidth - 100, 0, 100, 100,
                             view.x, 1.0f - view.y - view.h, view.w, view.h);
                break;

            case SpawnerPlacing:
                break;

            case EditorModeCount:
                unreachable();
        }
    }
}
