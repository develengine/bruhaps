#include "levels.h"

#include "terrain.h"
#include "core.h"
#include "utils.h"
#include "state.h"


Level level;


static ModelObject boxModel;

static unsigned pointProgram;


typedef enum {
    TerrainPlacing,
    TerrainHeightPainting,
    TerrainTexturing,
} EditorMode;

static EditorMode editorMode = TerrainTexturing;


static bool selected = false;
static int selectedX, selectedZ;
static int brushWidth = 2;

static uint8_t selectedViewID = 1;
static uint8_t selectedViewTrans = 0;


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

    // FIXME:
    levelBruhLoad();
}


void exitLevels(void)
{
    // FIXME:
    levelBruhUnload();

    levelExits();

    freeModelObject(boxModel);
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


void updateLevel(float dt)
{
    (void)dt;
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
        case TerrainHeightPainting:
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
    }
}


void levelsProcessWheel(bagE_MouseWheel *mw)
{
    switch (editorMode) {
        case TerrainPlacing:
            /* fallthrough */
        case TerrainHeightPainting:
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
            fromX = -(width / 2);
            toX   = width / 2 + width % 2;
            fromZ = -(height / 2);
            toZ   = height / 2 + height % 2;
        } else {
            fromZ = -brushWidth;
            toZ   =  brushWidth;
            fromX = -brushWidth;
            toX   =  brushWidth;
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
