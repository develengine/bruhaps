#include "levels.h"

#include "terrain.h"
#include "core.h"
#include "utils.h"
#include "state.h"


Level level;


static ModelObject boxModel;


#include "levels/bruh.c"


static void levelInits(void)
{
    for (LevelID id = 0; id < LevelCount; ++id) {
        switch (id) {
            case LevelBruh:
                levelBruhInit();
                break;
            case LevelCount:
                break;
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

    boxModel = createBoxModelObject();

    levelInits();

    // FIXME:
    levelBruhLoad();
}


void exitLevels(void)
{
    // FIXME:
    levelBruhUnload();

    freeModelObject(boxModel);
}


static bool selected = false;
static int selectedX, selectedZ;

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

            if (h < map->heights[map->chunkMap[cz * MAX_MAP_DIM + cx]].data[lz * CHUNK_DIM + lx]) {
                selected = true;
                selectedX = xp;
                selectedZ = zp;
                return;
            }
        }
    }
}


void updateLevel(float dt)
{
    (void)dt;
    /*
    selectVertex(
            camState.x, camState.y, camState.z,
            camState.pitch, camState.yaw,
            &level.terrain
    );
    */
}


void renderLevel(void)
{
    glDisable(GL_DEPTH_TEST);

    glUseProgram(level.skyboxProgram);
    glBindVertexArray(boxModel.vao);
    glBindTextureUnit(0, level.skyboxCubemap);

    glDrawElements(GL_TRIANGLES, BOX_INDEX_COUNT, GL_UNSIGNED_INT, 0);

    glEnable(GL_DEPTH_TEST);
}


void renderLevelDebugOverlay(void)
{
}
