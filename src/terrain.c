#include "terrain.h"

#include "utils.h"


ChunkMesh constructChunkMesh(Map *map, int cx, int cz)
{
    ChunkMesh mesh = { 0 };

    const int size = CHUNK_DIM * CHUNK_DIM;
    safe_expand(mesh.vertices, mesh.vertexCount, mesh.vertexCapacity, size);
    safe_expand(mesh.indices, mesh.indexCount, mesh.indexCapacity, size * 6);

    int chunk = map->chunkMap[cz * MAX_MAP_DIM + cx];
    float *heights = map->heights[chunk].data;

    float normalBuffer[(CHUNK_DIM * CHUNK_DIM + CHUNK_DIM * 2 + 1) * 3];

    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            float vecs[][3] = {
                { 0.0f, 666.f,-1.0f },
                {-1.0f, 666.f,-1.0f },
                {-1.0f, 666.f, 0.0f },
                {-1.0f, 666.f, 1.0f },
                { 0.0f, 666.f, 1.0f },
                { 1.0f, 666.f, 1.0f },
                { 1.0f, 666.f, 0.0f },
                { 1.0f, 666.f,-1.0f },
            };

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

            float nx = 0.0f, ny = 0.0f, nz = 0.0f;
            float height = heights[y * CHUNK_DIM + x];

            if (height == NO_TILE)
                continue;

            for (int i = 0; i < length(vecs); i++) {
                int xp = x + posses[i][1];
                int yp = y + posses[i][0];

                float res = atMapHeight(map, cx * CHUNK_DIM + xp, cz * CHUNK_DIM + yp);
                if (res == NO_TILE)
                    continue;

                vecs[i][1] = res - height;
            }

            for (int i = 0; i < length(vecs); ++i) {
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
/*
            int pos = (y * CHUNK_DIM + x) * 3;
            normalBuffer[pos + 0] = nx;
            normalBuffer[pos + 1] = ny;
            normalBuffer[pos + 2] = nz;
        }
    }

    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
*/
            Vertex vertex = {
                .positions = {
                    (float)x * CHUNK_TILE_DIM,
                    height,
                    (float)y * CHUNK_TILE_DIM
                },
                .normals = { nx, ny, nz },
                .textures = {
                    x % 2 ? 1.0f : 0.0f,
                    y % 2 ? 1.0f : 0.0f
                }
            };

            safe_push(mesh.vertices, mesh.vertexCount, mesh.vertexCapacity, vertex);
        }
    }

    for (int y = 0; y < CHUNK_DIM - 1; ++y) {
        for (int x = 0; x < CHUNK_DIM - 1; ++x) {
            int yo = y + 1;
            int xo = x + 1;
            safe_push(mesh.indices, mesh.indexCount, mesh.indexCapacity, y  * CHUNK_DIM + x);
            safe_push(mesh.indices, mesh.indexCount, mesh.indexCapacity, yo * CHUNK_DIM + x);
            safe_push(mesh.indices, mesh.indexCount, mesh.indexCapacity, yo * CHUNK_DIM + xo);

            safe_push(mesh.indices, mesh.indexCount, mesh.indexCapacity, y  * CHUNK_DIM + x);
            safe_push(mesh.indices, mesh.indexCount, mesh.indexCapacity, yo * CHUNK_DIM + xo);
            safe_push(mesh.indices, mesh.indexCount, mesh.indexCapacity, y  * CHUNK_DIM + xo);
        }
    }

    return mesh;
}

float atMapHeight(Map *map, int x, int z)
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

