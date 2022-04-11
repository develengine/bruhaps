#include "terrain.h"

#include "utils.h"


void constructChunkMesh(
        ChunkMesh *mesh,
        const Map *map,
        const AtlasView *atlasViews,
        int cx,
        int cz
) {
    mesh->vertexCount = 0;
    mesh->indexCount = 0;

    // FIXME: make static buffer sizes

    // const int normalDim = CHUNK_DIM + 1;
    float normalBuffer[(CHUNK_DIM + 1) * (CHUNK_DIM + 1) * 3];

    const float posses[][2] = {
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
            float height = atMapHeight(map, cx * CHUNK_DIM + x, cz * CHUNK_DIM + z);

            if (height == NO_TILE)
                continue;

            for (int i = 0; i < (int)length(vecs); i++) {
                int xp = x + posses[i][1];
                int zp = z + posses[i][0];

                float res = atMapHeight(map, cx * CHUNK_DIM + xp, cz * CHUNK_DIM + zp);
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

                    float height = atMapHeight(map, cx * CHUNK_DIM + xp, cz * CHUNK_DIM + zp);
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

            TileTexture texture = map->textures[cz * MAX_MAP_DIM + cx].data[z * CHUNK_DIM + x];
            AtlasView atlasView = atlasViews[texture.viewID];

            int indexOffset = mesh->vertexCount;

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

                    safe_push(mesh->vertices, mesh->vertexCount, mesh->vertexCapacity, vertex);
                }
            }

            safe_push(mesh->indices, mesh->indexCount, mesh->indexCapacity, indexOffset + 0);
            safe_push(mesh->indices, mesh->indexCount, mesh->indexCapacity, indexOffset + 2);
            safe_push(mesh->indices, mesh->indexCount, mesh->indexCapacity, indexOffset + 3);

            safe_push(mesh->indices, mesh->indexCount, mesh->indexCapacity, indexOffset + 3);
            safe_push(mesh->indices, mesh->indexCount, mesh->indexCapacity, indexOffset + 1);
            safe_push(mesh->indices, mesh->indexCount, mesh->indexCapacity, indexOffset + 0);

        }
    }
}


float atMapHeight(const Map *map, int x, int z)
{
    if (x < 0 || z < 0)
        return NO_TILE;

    int cx = x / CHUNK_DIM;
    int xp = x % CHUNK_DIM;
    int cz = z / CHUNK_DIM;
    int zp = z % CHUNK_DIM;

    if (cx >= MAX_MAP_DIM || cz >= MAX_MAP_DIM)
        return NO_TILE;

    if (map->chunkMap[cz * MAX_MAP_DIM + cx] == NO_CHUNK)
        return NO_TILE;

    if (xp >= CHUNK_DIM || zp >= CHUNK_DIM)
        return NO_TILE;

    return map->heights[cz * MAX_MAP_DIM + cx].data[zp * CHUNK_DIM + xp];
}

