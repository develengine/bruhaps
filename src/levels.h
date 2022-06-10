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


#define MOB_SPEED 3.0f
#define MOB_CHASE_RADIUS 48.0f
#define MOB_BITE_RANGE 1.0f
#define MOB_ATTACK_TO 1.0f


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
    bool emmited;
} Spawner;


#define MAX_SPAWNER_COUNT 256


typedef enum {
    NoGun,
    Glock,
    Gatling,

    GunTypeCount
} GunType;


typedef struct
{
    uint64_t vineThudLength;
    int16_t *vineThud;

    unsigned terrainProgram;
    unsigned terrainAtlas;
    
    ModelObject boxModel;
    unsigned skyboxProgram;
    unsigned skyboxCubemap;

    unsigned textureProgram;
    unsigned metalProgram;

    ModelObject gatling;
    ModelObject gatlingBase;
    ModelObject glock;
    ModelObject glockBase;
    unsigned gunTexture;

    GunType selectedGun;

    const AtlasView *atlasViews;

    const char *filePath;

    Terrain terrain;

    int chunkUpdateCount;
    unsigned chunkUpdates[MAX_MAP_DIM * MAX_MAP_DIM];

    int statsTypeCount;
    int          statsTypeOffsets [MAX_STATIC_TYPE_COUNT + 1];
    Object       statsTypeObjects [MAX_STATIC_TYPE_COUNT];
    ColliderType statsTypeCollider[MAX_STATIC_TYPE_COUNT];

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

    int spawnerCount;
    Spawner spawners[MAX_SPAWNER_COUNT];
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
void renderLevel(void);
void renderLevelOverlay(void);

void levelsSaveCurrent(void);

void requestChunkUpdate(unsigned chunkPos);
void invalidateAllChunks(void);

void levelsProcessButton(bagE_MouseButton *mb);
void levelsProcessWheel(bagE_MouseWheel *mw);

void levelsInsertStaticObject(Object object, ColliderType collider);
void levelsAddStatic(int statID, ModelTransform transform);

void staticsLoad(FILE *file);
void staticsSave(FILE *file);

void addMob(MobType type, ModelTransform trans, Animation anim);

#endif
