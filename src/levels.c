#include "levels.h"

#include "terrain.h"
#include "core.h"
#include "utils.h"
#include "state.h"


Level level;


static ModelObject boxModel;

static unsigned pointProgram;


typedef enum
{
    TerrainPlacing,
    TerrainTexturing,
    TerrainHeightPaintingAbs,
    TerrainHeightPaintingRel,
    StaticsPlacing,
} EditorMode;

static EditorMode editorMode = StaticsPlacing;


static bool selected = false;
static int selectedX, selectedZ;
static int brushWidth = 2;

static uint8_t selectedViewID = 1;
static uint8_t selectedViewTrans = 0;

static float selectedRelHeight = 0.1f;
static float selectedAbsHeight = 3.0f;

static int selectedStaticID = 0;


// TODO remove me
static Model gatlingModel;
static ModelObject gatling;
static unsigned metalProgram;
static float timePassed = 0.0f;

static unsigned staticProgram;
static unsigned staticUBO;
static Matrix staticMatrixBuffer[MAX_STATIC_INSTANCE_COUNT];


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

    boxModel = createBoxModelObject();

    levelInits();


    // FIXME: at least free me
    gatlingModel  = modelLoad("res/gatling_barrel.model");
    gatling = createModelObject(gatlingModel);

    metalProgram = createProgram(
            "shaders/metal_vertex.glsl",
            "shaders/metal_fragment.glsl"
    );


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


    // FIXME:
    levelLoad(LevelBruh);
}


void exitLevels(void)
{
    // FIXME:
    levelUnload(LevelBruh);

    levelExits();

    freeModelObject(boxModel);
}


void levelsInsertStaticObject(Object object, Collider collider)
{
    level.statsObjects  [level.statsTypeCount] = object;
    level.statsColliders[level.statsTypeCount] = collider;
    ++level.statsTypeCount;
}


void levelsAddStatic(int statID, ModelTransform transform)
{
    assert(level.statsInstanceCount < MAX_STATIC_INSTANCE_COUNT);

    ++level.statsInstanceCount;
    level.recalculateStats = true;

    while (++statID <= level.statsTypeCount) {
        int oldOffset = level.statsOffsets[statID]++;
        ModelTransform oldTransform = level.statsTransforms[oldOffset];

        level.statsTransforms[oldOffset] = transform;
        transform = oldTransform;
    }
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


// FIXME: a magic function
void processPlayerInput(float vx, float vz, bool jump, float dt)
{
    (void)vx, (void)vz;

    float groundTolerance = 0.1f;
    float playerHeight = 2.0f;

    float oldY = playerState.y;

    float newX = playerState.x + vx;
    float newZ = playerState.z + vz;
    float newY = getHeight(newX, newZ);

    float distH = sqrtf(vx * vx + vz * vz);
    float distV = (newY + playerHeight) - oldY;

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

    playerState.x += vx;
    playerState.z += vz;

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
         * we skip to avoid divide by 0 */
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

    /* update stats */
    if (level.recalculateStats) {
        level.recalculateStats = false;

        for (int i = 0; i < level.statsInstanceCount; ++i) {
            ModelTransform transform = level.statsTransforms[i];

            Matrix mod = matrixScale(transform.scale, transform.scale, transform.scale);

            Matrix mul = matrixRotationZ(transform.rz);
            mod = matrixMultiply(&mul, &mod);

            mul = matrixRotationY(transform.ry);
            mod = matrixMultiply(&mul, &mod);

            mul = matrixRotationX(transform.rx);
            mod = matrixMultiply(&mul, &mod);

            mul = matrixTranslation(transform.x, transform.y, transform.z);
            mod = matrixMultiply(&mul, &mod);

            staticMatrixBuffer[i] = mod;
        }

        glNamedBufferSubData(
                staticUBO,
                0,
                sizeof(Matrix) * level.statsInstanceCount,
                staticMatrixBuffer
        );
    }
}


void renderLevel(void)
{
    /* render skybox */
    glDisable(GL_DEPTH_TEST);

    glUseProgram(level.skyboxProgram);
    glBindVertexArray(boxModel.vao);
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


    glUseProgram(metalProgram);
    glBindVertexArray(gatling.vao);

    glProgramUniformMatrix4fv(metalProgram, 0, 1, GL_FALSE, modelGatling.data);

    glDrawElements(GL_TRIANGLES, gatlingModel.indexCount, GL_UNSIGNED_INT, 0);

    /* render statics */
    glUseProgram(staticProgram);

    for (int i = 0; i < level.statsTypeCount; ++i) {
        Object object = level.statsObjects[i];
        glBindVertexArray(object.model.vao);
        glBindTextureUnit(0, object.texture);

        glProgramUniform1ui(staticProgram, 0, level.statsOffsets[i]);

        glDrawElementsInstanced(
                GL_TRIANGLES,
                object.model.indexCount,
                GL_UNSIGNED_INT,
                0,
                getStaticCount(i)
        );
    }
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
            if (mb->button == bagE_ButtonLeft) {
                TileTexture tileTex = {
                    .viewID = selectedViewID,
                };
                setTerrainTexture(&level.terrain, selectedX, selectedZ, tileTex);
                updateNearbyChunks(selectedX, selectedZ);
            }
            break;
        case StaticsPlacing:
            if (mb->button == bagE_ButtonLeft) {
                levelsAddStatic(selectedStaticID, (ModelTransform) {
                    .x = selectedX * CHUNK_TILE_DIM,
                    .y = atTerrainHeight(&level.terrain, selectedX, selectedZ),
                    .z = selectedZ * CHUNK_TILE_DIM,
                    .scale = 1.0f,
                });
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
        case TerrainTexturing:
            break;
    }
}


void levelsSaveCurrent(void)
{
    assert(level.filePath != NULL);

    FILE *file = fopen(level.filePath, "wb");
    file_check(file, level.filePath);

    terrainSave(&level.terrain, file);

    fclose(file);

    printf("saved \"%s\".\n", level.filePath);
}


void renderLevelDebugOverlay(void)
{
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

        glProgramUniform4fv(pointProgram, 1,  pointCount, colors->data);
        glProgramUniform4fv(pointProgram, 33, pointCount, points->data);

        glDrawArraysInstanced(GL_POINTS, 0, 1, pointCount);

    }
}
