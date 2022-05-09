#include "terrain.h"

#include "bag_engine.h"
#include "utils.h"

#define CHUNK_VERTEX_COUNT (CHUNK_DIM * CHUNK_DIM * 4)
#define CHUNK_INDEX_COUNT  (CHUNK_DIM * CHUNK_DIM * 6)

static Vertex   vertexBuffer[CHUNK_VERTEX_COUNT];
static unsigned indexBuffer[CHUNK_INDEX_COUNT];
static float    normalBuffer[(CHUNK_DIM + 1) * (CHUNK_DIM + 1) * 3];

void updateChunkObject(
        ChunkObject *chunkObject,
        const Terrain *terrain,
        const AtlasView *atlasViews,
        int cx,
        int cz
) {
    int vertexCount = 0;
    int indexCount = 0;

    const int posses[][2] = {
    //    y, x
        {-1, 0 },
        {-1,-1 },
        { 0,-1 },
        { 1,-1 },
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        {-1, 1 },
    };

    float vecs[][3] = {
        { 0.0f,           NO_TILE,-CHUNK_TILE_DIM },
        {-CHUNK_TILE_DIM, NO_TILE,-CHUNK_TILE_DIM },
        {-CHUNK_TILE_DIM, NO_TILE, 0.0f },
        {-CHUNK_TILE_DIM, NO_TILE, CHUNK_TILE_DIM },
        { 0.0f,           NO_TILE, CHUNK_TILE_DIM },
        { CHUNK_TILE_DIM, NO_TILE, CHUNK_TILE_DIM },
        { CHUNK_TILE_DIM, NO_TILE, 0.0f },
        { CHUNK_TILE_DIM, NO_TILE,-CHUNK_TILE_DIM },
    };

    /* normals */
    for (int z = 0; z < CHUNK_DIM + 1; ++z) {
        for (int x = 0; x < CHUNK_DIM + 1; ++x) {
            float nx = 0.0f, ny = 0.0f, nz = 0.0f;
            float height = atTerrainHeight(terrain, cx * CHUNK_DIM + x, cz * CHUNK_DIM + z);

            if (height == NO_TILE)
                continue;

            for (int i = 0; i < (int)length(vecs); i++) {
                int xp = x + posses[i][1];
                int zp = z + posses[i][0];

                float res = atTerrainHeight(terrain, cx * CHUNK_DIM + xp, cz * CHUNK_DIM + zp);
                if (res == NO_TILE) {
                    vecs[i][1] = res;
                    continue;
                }

                vecs[i][1] = res - height;
            }

            for (int i = 0; i < (int)length(vecs); ++i) {
                float norm[3];
                float *a = vecs[i];
                float *b = vecs[(i + 1) % length(vecs)];

                if (a[1] == 666.f || b[1] == 666.f)
                    continue;

                cross(norm, a, b);

                float invLen = 1.0f / vecLen(norm);
                norm[0] *= invLen;
                norm[1] *= invLen;
                norm[2] *= invLen;

                float weight = acosf(dot(a, b) / (vecLen(a) * vecLen(b)));

                nx += norm[0] * weight;
                ny += norm[1] * weight;
                nz += norm[2] * weight;
            }

            float invLen = 1.0f / sqrtf(nx * nx + ny * ny + nz * nz);
            nx *= invLen;
            ny *= invLen;
            nz *= invLen;

            int pos = (z * (CHUNK_DIM + 1) + x) * 3;
            normalBuffer[pos + 0] = nx;
            normalBuffer[pos + 1] = ny;
            normalBuffer[pos + 2] = nz;
        }
    }

    /* vertices */
    for (int z = 0; z < CHUNK_DIM; ++z) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            float *normals[4];
            float heights[4];

            bool discardTile = true;

            for (int zi = 0; zi < 2; ++zi) {
                for (int xi = 0; xi < 2; ++xi) {
                    int xp = x + xi;
                    int zp = z + zi;

                    float height = atTerrainHeight(terrain, cx * CHUNK_DIM + xp, cz * CHUNK_DIM + zp);
                    if (height == NO_TILE)
                        goto discard_tile;

                    heights[zi * 2 + xi] = height;
                    normals[zi * 2 + xi] = normalBuffer + (zp * (CHUNK_DIM + 1) + xp) * 3;
                }
            }

            discardTile = false;
discard_tile:
            if (discardTile)
                continue;

            int chunkID = terrain->chunkMap[cz * MAX_MAP_DIM + cx];

            TileTexture texture = terrain->textures[chunkID]->data[z * CHUNK_DIM + x];
            AtlasView atlasView = atlasViews[texture.viewID];

            int indexOffset = vertexCount;

            for (int zi = 0; zi < 2; ++zi) {
                for (int xi = 0; xi < 2; ++xi) {
                    float *n = normals[zi * 2 + xi];

                    Vertex vertex = {
                        .positions = {
                            (float)(x + xi) * CHUNK_TILE_DIM,
                            heights[zi * 2 + xi],
                            (float)(z + zi) * CHUNK_TILE_DIM
                        },
                        .normals = { n[0], n[1], n[2] },
                        .textures = {
                            atlasView.x + (texture.x + xi) * (atlasView.w / atlasView.wn),
                            atlasView.y + (texture.y + zi) * (atlasView.h / atlasView.hn),
                        }
                    };

                    vertexBuffer[vertexCount++] = vertex;
                }
            }

            indexBuffer[indexCount++] = indexOffset + 0;
            indexBuffer[indexCount++] = indexOffset + 2;
            indexBuffer[indexCount++] = indexOffset + 3;

            indexBuffer[indexCount++] = indexOffset + 3;
            indexBuffer[indexCount++] = indexOffset + 1;
            indexBuffer[indexCount++] = indexOffset + 0;

        }
    }

    glNamedBufferSubData(chunkObject->vbo, 0, vertexCount * sizeof(Vertex),   vertexBuffer);
    glNamedBufferSubData(chunkObject->ebo, 0, indexCount  * sizeof(unsigned), indexBuffer);
    chunkObject->vertexCount = vertexCount;
    chunkObject->indexCount  = indexCount ;
}


float atTerrainHeight(const Terrain *terrain, int x, int z)
{
    if (x < 0 || z < 0)
        return NO_TILE;

    int cx = x / CHUNK_DIM;
    int xp = x % CHUNK_DIM;
    int cz = z / CHUNK_DIM;
    int zp = z % CHUNK_DIM;

    if (cx >= MAX_MAP_DIM || cz >= MAX_MAP_DIM)
        return NO_TILE;

    int chunkID;

    if ((chunkID = terrain->chunkMap[cz * MAX_MAP_DIM + cx]) == NO_CHUNK)
        return NO_TILE;

    return terrain->heights[chunkID]->data[zp * CHUNK_DIM + xp];
}


static TerrainLocation maybeCreateChunk(Terrain *terrain, int x, int z)
{
    TerrainLocation location = { .chunkID = NO_CHUNK };

    if (x < 0 || z < 0)
        return location;

    int cx = x / CHUNK_DIM;
    int xp = x % CHUNK_DIM;
    int cz = z / CHUNK_DIM;
    int zp = z % CHUNK_DIM;

    if (cx >= MAX_MAP_DIM || cz >= MAX_MAP_DIM)
        return location;

    int chunkPos = cz * MAX_MAP_DIM + cx;
    int chunkID;

    /* create new chunk */
    if ((chunkID = terrain->chunkMap[chunkPos]) == NO_CHUNK) {
        chunkID = terrain->chunkCount++;
        terrain->chunkMap[chunkPos] = chunkID;

        ChunkHeights  *chunkHeights  = malloc(sizeof(ChunkHeights));
        ChunkTextures *chunkTextures = malloc(sizeof(ChunkTextures));
        malloc_check(chunkHeights);
        malloc_check(chunkTextures);

        for (int i = 0; i < CHUNK_DIM * CHUNK_DIM; ++i)
            chunkHeights->data[i] = NO_TILE;

        for (int i = 0; i < CHUNK_DIM * CHUNK_DIM; ++i)
            chunkTextures->data[i] = (TileTexture) { 0, 0, 0, 0 };

        terrain->heights [chunkID] = chunkHeights;
        terrain->textures[chunkID] = chunkTextures;
        terrain->objects [chunkID] = createChunkObject();
    }

    location.chunkID = chunkID;
    location.xp = xp;
    location.zp = zp;

    return location;
}


void setTerrainHeight(Terrain *terrain, int x, int z, float height)
{
    TerrainLocation loc = maybeCreateChunk(terrain, x, z);
    if (loc.chunkID == NO_CHUNK)
        return;

    terrain->heights[loc.chunkID]->data[loc.zp * CHUNK_DIM + loc.xp] = height;
}


void setTerrainTexture(Terrain *terrain, int x, int z, TileTexture tileTexture)
{
    TerrainLocation loc = maybeCreateChunk(terrain, x, z);
    if (loc.chunkID == NO_CHUNK)
        return;

    terrain->textures[loc.chunkID]->data[loc.zp * CHUNK_DIM + loc.xp] = tileTexture;
}


void terrainClearChunkMap(Terrain *terrain)
{
    for (int i = 0; i < MAX_MAP_DIM * MAX_MAP_DIM; ++i)
        terrain->chunkMap[i] = NO_CHUNK;
}


void terrainLoad(Terrain *terrain, FILE *file)
{
    terrainClearChunkMap(terrain);

    char buffer[4];
    safe_read(buffer, 1, 4, file);
    if (strncmp(buffer, "TERR", 4)) {
        fprintf(stderr, "Can't parse terrain file!\n");
        exit(666);
    }

    safe_read(&terrain->chunkCount, sizeof(int), 1, file);

    for (int i = 0; i < terrain->chunkCount; ++i) {
        int cx, cz;
        safe_read(&cx, sizeof(int), 1, file);
        safe_read(&cz, sizeof(int), 1, file);
        terrain->chunkMap[cz * MAX_MAP_DIM + cx] = i;

        ChunkHeights *heights = malloc(sizeof(ChunkHeights));
        malloc_check(heights);
        safe_read(heights, sizeof(ChunkHeights), 1, file);
        terrain->heights[i] = heights;

        ChunkTextures *textures = malloc(sizeof(ChunkTextures));
        malloc_check(textures);
        safe_read(textures, sizeof(ChunkTextures), 1, file);
        terrain->textures[i] = textures;

        terrain->objects[i] = createChunkObject();
    }
}


void terrainSave(Terrain *terrain, FILE *file)
{
    safe_write("TERR", 1, 4, file);
    safe_write(&terrain->chunkCount, sizeof(int), 1, file);

    for (int cz = 0; cz < MAX_MAP_DIM; ++cz) {
        for (int cx = 0; cx < MAX_MAP_DIM; ++cx) {
            int chunkID = terrain->chunkMap[cz * MAX_MAP_DIM + cx];
            if (chunkID != NO_CHUNK) {
                safe_write(&cx, sizeof(int), 1, file);
                safe_write(&cz, sizeof(int), 1, file);

                ChunkHeights *heights = terrain->heights[chunkID];
                safe_write(heights, sizeof(ChunkHeights), 1, file);

                ChunkTextures *textures = terrain->textures[chunkID];
                safe_write(textures, sizeof(ChunkTextures), 1, file);
            }
        }
    }
}


ChunkObject createChunkObject(void)
{
    ChunkObject object;

    glCreateBuffers(1, &object.vbo);
    glNamedBufferStorage(
            object.vbo,
            CHUNK_VERTEX_COUNT * sizeof(Vertex),
            NULL,
            GL_DYNAMIC_STORAGE_BIT
    );

    glCreateBuffers(1, &object.ebo);
    glNamedBufferStorage(
            object.ebo,
            CHUNK_INDEX_COUNT * sizeof(unsigned),
            NULL,
            GL_DYNAMIC_STORAGE_BIT
    );

    glCreateVertexArrays(1, &object.vao);
    glVertexArrayVertexBuffer(object.vao, 0, object.vbo, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(object.vao, 0);
    glEnableVertexArrayAttrib(object.vao, 1);
    glEnableVertexArrayAttrib(object.vao, 2);

    glVertexArrayAttribFormat(object.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(object.vao, 1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
    glVertexArrayAttribFormat(object.vao, 2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5);

    glVertexArrayAttribBinding(object.vao, 0, 0);
    glVertexArrayAttribBinding(object.vao, 1, 0);
    glVertexArrayAttribBinding(object.vao, 2, 0);

    glVertexArrayElementBuffer(object.vao, object.ebo);

    return object;
}
