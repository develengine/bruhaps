#include "levels.h"

#include "terrain.h"
#include "core.h"
#include "utils.h"
#include "state.h"


Level level;


static ModelObject boxModel;

static unsigned pointProgram;

static bool selected = false;
static int selectedX, selectedZ;


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


// TODO: REMOVE THIS CRAP
// FIXME: make static
void selectVertex(
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

        if (xp >= 0 && cx < MAX_MAP_DIM
         && zp >= 0 && cz < MAX_MAP_DIM
         && map->chunkMap[cz * MAX_MAP_DIM + cx] != NO_CHUNK) {
            int dx = (int)(x - camXS);
            int dz = (int)(z - camZS);
            float h = camY + sqrtf((float)(dx * dx + dz * dz)) * yx;

            int lx = xp % CHUNK_DIM;
            int lz = zp % CHUNK_DIM;

            if (h < map->heights[map->chunkMap[cz * MAX_MAP_DIM + cx]]->data[lz * CHUNK_DIM + lx]) {
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
        int chunkID  = level.terrain.chunkMap[chunkPos];
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


void renderLevelDebugOverlay(void)
{
    /* point */
    if (selected) {
        glUseProgram(pointProgram);

        float colors[] = {
            0.5f, 1.0f, 0.75f, 1.0f,
        };

        float points[] = {
            selectedX * CHUNK_TILE_DIM,
            atTerrainHeight(&level.terrain, selectedX, selectedZ),
            selectedZ * CHUNK_TILE_DIM,
            18.0f,
        };

        glProgramUniform4fv(pointProgram, 1, 1, colors);
        glProgramUniform4fv(pointProgram, 33, 1, points);

        glDrawArraysInstanced(GL_POINTS, 0, 1, 1);
    }
}
