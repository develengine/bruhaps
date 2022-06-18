#ifndef LEVELS_H
#define LEVELS_H

#include "terrain.h"
#include "bag_engine.h"
#include "core.h"
#include "state.h"
#include "animation.h"

#include <stdbool.h>


typedef enum
{
    LevelBruh,

    LevelCount
} LevelID;


#define MAX_STATIC_TYPE_COUNT 128
/* NOTE: reflected in shaders/static_vertex.glsl */
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
    union {
        struct { float x, y, z; };
        float pos[3];
    };
    union {
        struct { float sx, sy, sz; };
        float size[3];
    };
    union {
        struct { float rx, ry, rz; };
        float rot[3];
    };
} Collider;


typedef struct
{
    bool inGame;
    Collider collider;
} ColliderType;


typedef struct
{
    ModelObject model;
    unsigned texture;
} Object;


typedef struct
{
    AnimatedObject animated;
    unsigned texture;
} MobObject;


typedef struct
{
    int offset;
    int count;
} ChunkColliderID;


typedef enum
{
    MobWorm,

    MobCount
} MobType;

/* NOTE: reflected in shaders/animated_vertex.glsl */
#define MOB_BONE_POOL_SIZE 1024
#define MAX_BONES_PER_MOB  128
#define MAX_MOBS_PER_TYPE  64

typedef enum
{
    MobStateWalking,
    MobStateIdle,

    MobStateCount
} MobState;


#define MOB_SPEED 5.0f
#define MOB_CHASE_RADIUS 32.0f
#define MOB_BITE_RANGE 2.0f
#define MOB_ATTACK_TO 1.0f
#define MOB_THICKNESS 1.0f


typedef enum
{
    SpawnerInit,

    SpawnerGroupCount
} SpawnerGroup;

#define SpawnerGroupAll -1


typedef struct
{
    MobType type;
    SpawnerGroup group;
    float x, y, z;
    bool emitted;
} Spawner;


#define MAX_SPAWNER_COUNT 256


typedef enum
{
    Glock,
    Gatling,

    GunTypeCount
} GunType;


#define GLOCK_BUMP_TIME 0.16f
#define GATLING_SPINUP 1.5f
#define GATLING_SPIN_SPEED 1.5f
#define GATLING_FIRE_RATE 16.0f


typedef enum
{
    HealthPickup,
    AmmoPickup,
    HeadPickup,

    PickupCount
} Pickup;


#define MAX_PICKUP_COUNT 256
#define PICKUP_SPEED 2.0f
#define PICKUP_RADIUS 1.5f

typedef bool (*PickupAction)(void);


#define WALK_LENGTH 0.3f


typedef struct
{
    // TODO: This stuff should not be here but for
    //       now it's fine
    //       This whole file should be named something
    //       more like 'game' rather then 'levels'
    uint64_t vineThudLength;
    int16_t *vineThud;
    uint64_t bite87Length;
    int16_t *bite87;
    uint64_t ssAmmoPickupLength;
    int16_t *ssAmmoPickup;
    uint64_t bruh2Length;
    int16_t *bruh2;
    uint64_t gulpLength;
    int16_t *gulp;
    uint64_t stoneHitLength;
    int16_t *stoneHit;
    uint64_t happyWheelsWinLength;
    int16_t *happyWheelsWin;
    uint64_t steveRevLength;
    int16_t *steveRev;
    uint64_t landLength;
    int16_t *land;

    float walkTime;
    bool inJump;

    unsigned guiAtlas;

    unsigned terrainProgram;
    unsigned terrainAtlas;
    
    ModelObject boxModel;
    unsigned skyboxProgram;
    unsigned skyboxCubemap;

    unsigned textureProgram;

    /* NOTE: exists purely because of 2 light requirement */
    unsigned lightProgram;

    ModelObject platform;
    unsigned platformTexture;

    unsigned metalProgram;
    ModelObject gatling;
    ModelObject gatlingBase;
    ModelObject glock;
    ModelObject glockBase;
    unsigned gunTexture;
    float gunTime;
    float gatlingSpeed;
    float gatlingTO;
    int gatlingAmmo;

    GunType selectedGun;

    ModelObject head;
    unsigned headTexture;
    int headCount;
    int carryHeadCount;

    const AtlasView *atlasViews;
    int atlasViewCount;

    const char *filePath;

    Terrain terrain;

    int chunkUpdateCount;
    unsigned chunkUpdates[MAX_MAP_DIM * MAX_MAP_DIM];

    int statsTypeCount;
    int          statsTypeOffsets [MAX_STATIC_TYPE_COUNT + 1];
    Object       statsTypeObjects [MAX_STATIC_TYPE_COUNT];
    ColliderType statsTypeCollider[MAX_STATIC_TYPE_COUNT];
    const char  *statsTypeName    [MAX_STATIC_TYPE_COUNT];

    bool recalculateStats;
    int  statsInstanceCount;
    ModelTransform statsTransforms[MAX_STATIC_INSTANCE_COUNT + 1];

    bool recalculateStatsColliders;
    int  statsColliderCount;
    int      statsColliderOffsetMap[MAX_MAP_DIM * MAX_MAP_DIM + 1];
    Collider statsColliders        [MAX_STATIC_INSTANCE_COUNT];

    int            mobTypeCounts[MobCount];
    Armature       mobArmatures [MobCount];
    MobObject      mobObjects   [MobCount];
    Animation      mobAnimations[MobCount * MAX_MOBS_PER_TYPE];
    ModelTransform mobTransforms[MobCount * MAX_MOBS_PER_TYPE];
    MobState       mobStates    [MobCount * MAX_MOBS_PER_TYPE];
    float          mobAttackTOs [MobCount * MAX_MOBS_PER_TYPE];
    int            mobHPs       [MobCount * MAX_MOBS_PER_TYPE];

    int spawnerCount;
    Spawner spawners[MAX_SPAWNER_COUNT];

    int pickupCount;
    Pickup pickups        [MAX_PICKUP_COUNT];
    Vector pickupPositions[MAX_PICKUP_COUNT];
    float pickupTime;
    Object      pickupObjects[PickupCount];
    const char *pickupNames  [PickupCount];
} Level;

extern Level level;


#define PLAYER_HP_FULL 100


static inline int getStaticCount(int staticID)
{
    return level.statsTypeOffsets[staticID + 1] - level.statsTypeOffsets[staticID];
}


static inline int getStaticChunkColliderCount(int chunkPos)
{
    return level.statsColliderOffsetMap[chunkPos + 1] - level.statsColliderOffsetMap[chunkPos];
}


Matrix modelTransformToMatrix(ModelTransform transform);

void initLevels(void);
void exitLevels(void);

void processPlayerInput(float vx, float vz, bool jump, float dt);

void updateLevel(float dt);
void updateMenu(float dt);
void menuProcessMouse(bagE_Mouse *m);
void renderLevel(void);
void renderLevelOverlay(void);
void processEsc(void);

void levelsSaveCurrent(void);

void requestChunkUpdate(unsigned chunkPos);
void invalidateAllChunks(void);

void levelsProcessButton(bagE_MouseButton *mb, bool down);
void levelsProcessWheel(bagE_MouseWheel *mw);

void levelsInsertStaticObject(Object object, ColliderType collider, const char *name);
void levelsAddStatic(int statID, ModelTransform transform);

void staticsLoad(FILE *file);
void staticsSave(FILE *file);

void spawnersLoad(FILE *file);
void spawnersSave(FILE *file);

void pickupsLoad(FILE *file);
void pickupsSave(FILE *file);

void addMob(MobType type, ModelTransform trans, Animation anim);
void removeMob(MobType type, int index);

void addSpawner(Spawner spawner);
void removeSpawner(int index);
void spawnersBroadcast(SpawnerGroup group);

int playerRaySelect(void);
void playerShoot(int damage);

void addPickup(Pickup pickup, Vector pos);
void removePickup(int index);

void restartLevel(void);

void levelLoad(LevelID id);
void levelUnload(LevelID id);

#endif
