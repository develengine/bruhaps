#include "levels.h"

#include "terrain.h"
#include "core.h"
#include "utils.h"
#include "state.h"
#include "audio.h"
#include "gui.h"


Level level;


static unsigned pointProgram;


typedef enum
{
    TerrainPlacing,
    TerrainTexturing,
    TerrainHeightPaintingAbs,
    TerrainHeightPaintingRel,
    StaticsPlacing,
    TestMorbing,
} EditorMode;

static EditorMode editorMode = TestMorbing;


static bool selected = false;
static int selectedX, selectedZ;
static int brushWidth = 2;

static uint8_t selectedViewID = 1;
static uint8_t selectedViewTrans = 0;

static float selectedRelHeight = 0.1f;
static float selectedAbsHeight = 3.0f;

static int selectedStaticID = 0;


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


static void levelLoad(LevelID id)
{
    switch (id) {
        case LevelBruh:  levelBruhLoad(); break;
        case LevelCount: break;
    }
}


static void levelUnload(LevelID id)
{
    switch (id) {
        case LevelBruh:  levelBruhUnload(); break;
        case LevelCount: break;
    }

    level.filePath = NULL;
}


void initLevels(void)
{
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


    level.vineThud = loadWAV("res/vine_thud.wav", &level.vineThudLength);


    playerState.hp = PLAYER_HP_FULL;


    // FIXME:
    levelLoad(LevelBruh);
}


void exitLevels(void)
{
    // FIXME:
    levelUnload(LevelBruh);

    levelExits();

    freeModelObject(level.boxModel);
}


void levelsInsertStaticObject(Object object, ColliderType collider)
{
    assert(level.statsTypeCount < MAX_STATIC_TYPE_COUNT);

    level.statsTypeObjects [level.statsTypeCount] = object;
    level.statsTypeCollider[level.statsTypeCount] = collider;
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

    printf("%d\n", level.statsInstanceCount);

    for (int i = typeCount; i <= MAX_STATIC_TYPE_COUNT; ++i)
        level.statsTypeOffsets[i] = level.statsInstanceCount;

    level.recalculateStats          = true;
    level.recalculateStatsColliders = true;
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

    ++level.mobTypeCounts[type];
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

    if (playerState.y < height + groundTolerance) {
        playerState.y = height;
        playerState.vy = 0.0f;
        playerState.onGround = true;
    } else {
        playerState.onGround = false;
    }

    if (jump && playerState.onGround) {
        playerState.vy += 5.0f;
        playerState.y += groundTolerance;
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
                if (level.mobAttackTOs[mobID] == 0.0f) {
                    level.mobAttackTOs[mobID] = MOB_ATTACK_TO;
                    playerState.hp -= PLAYER_HP_FULL / 5;
                    if (playerState.hp < 0)
                        playerState.hp = 0;
                }
            } else if (toPlayerDistance < MOB_CHASE_RADIUS) {
                toPlayerX /= toPlayerDistance;
                toPlayerZ /= toPlayerDistance;

                level.mobTransforms[mobID].x += toPlayerX * MOB_SPEED * dt;
                level.mobTransforms[mobID].z += toPlayerZ * MOB_SPEED * dt;
                level.mobTransforms[mobID].y = getHeight(level.mobTransforms[mobID].x,
                                                         level.mobTransforms[mobID].z);

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

    /* model gatling */
    Matrix modelGatling = matrixScale(1.0f, 1.0f, 1.0f);

    Matrix mul = matrixRotationZ(timePassed * 2.0f);
    modelGatling = matrixMultiply(&mul, &modelGatling);

    mul = matrixRotationY(0.0f);
    modelGatling = matrixMultiply(&mul, &modelGatling);

    mul = matrixTranslation(-10.0f, 0.0f, 0.0f);
    modelGatling = matrixMultiply(&mul, &modelGatling);


    glUseProgram(level.metalProgram);
    glBindVertexArray(level.gatling.vao);

    glProgramUniformMatrix4fv(level.metalProgram, 0, 1, GL_FALSE, modelGatling.data);

    glDrawElements(GL_TRIANGLES, level.gatling.indexCount, GL_UNSIGNED_INT, 0);

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

#if 1
    /* debug render static colliders */
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
    glProgramUniform4fv(pointProgram, 33, 8, points2->data);

    for (int i = 0; i < level.statsColliderCount; ++i) {
        Collider c = level.statsColliders[i];

        Matrix modMat = matrixScale(c.sx, c.sy, c.sz);
        mul = matrixTranslation(c.x, c.y, c.z);
        modMat = matrixMultiply(&mul, &modMat);

        glProgramUniformMatrix4fv(pointProgram, 0, 1, false, modMat.data);
        glDrawArraysInstanced(GL_POINTS, 0, 1, 8);
    }
#endif
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


void levelsProcessButton(bagE_MouseButton *mb)
{
    switch (editorMode) {
        case TerrainPlacing:
            if (mb->button == bagE_ButtonLeft) {
                extendHeights();
            } else if (mb->button == bagE_ButtonRight) {
                removeTiles();
            }
            break;
        case TerrainHeightPaintingAbs:
            /* fallthrough */
        case TerrainHeightPaintingRel:
            if (mb->button == bagE_ButtonLeft) {
                scaleHeights(1.0f);
            } else if (mb->button == bagE_ButtonRight) {
                scaleHeights(-1.0f);
            }
            break;
        case TerrainTexturing:
            if (mb->button == bagE_ButtonLeft && selected) {
                TileTexture tileTex = {
                    .viewID = selectedViewID,
                };
                setTerrainTexture(&level.terrain, selectedX, selectedZ, tileTex);
                updateNearbyChunks(selectedX, selectedZ);
            }
            break;
        case StaticsPlacing:
            if (mb->button == bagE_ButtonLeft && selected) {
                levelsAddStatic(selectedStaticID, (ModelTransform) {
                    .x = selectedX * CHUNK_TILE_DIM,
                    .y = atTerrainHeight(&level.terrain, selectedX, selectedZ),
                    .z = selectedZ * CHUNK_TILE_DIM,
                    .scale = 1.0f,
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
        case TestMorbing:
            if (mb->button == bagE_ButtonLeft && selected) {
                float x = selectedX * CHUNK_TILE_DIM;
                float z = selectedZ * CHUNK_TILE_DIM;
                addMob(MobWorm, (ModelTransform) { x, getHeight(x, z), z, 2.0f },
                                (Animation) { .start = level.mobArmatures[MobWorm].timeStamps[0],
                                              .end   = level.mobArmatures[MobWorm].timeStamps[2],
                                              .time  = 0.0f });
            }
            break;
    }
}


void levelsProcessWheel(bagE_MouseWheel *mw)
{
    switch (editorMode) {
        case TerrainPlacing:
            /* fallthrough */
        case TerrainHeightPaintingAbs:
            /* fallthrough */
        case TerrainHeightPaintingRel:
            brushWidth += mw->scrollUp;
            if (brushWidth < 0)
                brushWidth = 0;
            if (brushWidth > 2)
                brushWidth = 2;
            break;
        case StaticsPlacing:
            selectedStaticID += mw->scrollUp;
            if (selectedStaticID < 0)
                selectedStaticID = 0;
            if (selectedStaticID >= level.statsTypeCount)
                selectedStaticID = level.statsTypeCount - 1;
            break;
    }
}


void levelsSaveCurrent(void)
{
    assert(level.filePath != NULL);

    FILE *file = fopen(level.filePath, "wb");
    file_check(file, level.filePath);

    terrainSave(&level.terrain, file);
    staticsSave(file);

    fclose(file);

    printf("saved \"%s\".\n", level.filePath);
}


void renderLevelOverlay(void)
{
    if (playerState.gaming) {
        Vector healthColor = {{ 1.0f, 0.0f, 0.0f, 1.0f }};
        Vector backColor   = {{ 0.2f, 0.0f, 0.0f, 1.0f }};

        int barWidth  = appState.windowWidth / 5;
        int barHeight = barWidth / 10;
        int margin    = barHeight * 2;
        int padding   = 3;

        guiBeginRect();
        guiDrawRect(margin, appState.windowHeight - margin - barHeight,
                    barWidth, barHeight,
                    backColor);
        guiDrawRect(margin + padding,
                    appState.windowHeight - margin - barHeight + padding,
                    (int)(((float)barWidth / PLAYER_HP_FULL) * playerState.hp) - 2 * padding,
                    barHeight - 2 * padding,
                    healthColor);
    } else {
        /* point */
        if (selected) {
            glUseProgram(pointProgram);

            int pointCount = 0;

            Vector colors[32];
            Vector points[32];

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
                    || editorMode == TerrainHeightPaintingRel
                    || editorMode == TerrainHeightPaintingAbs) {
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
            glProgramUniform4fv(pointProgram, 33, pointCount, points->data);

            glDrawArraysInstanced(GL_POINTS, 0, 1, pointCount);
        }
    }
}
