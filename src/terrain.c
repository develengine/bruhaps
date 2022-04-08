#include "terrain.h"

#include "utils.h"


ChunkMesh constructChunkMesh(Map *map, int chunk)
{
    ChunkMesh mesh = { 0 };

    const int size = CHUNK_DIM * CHUNK_DIM;
    safe_expand(mesh.vertices, mesh.vertexCount, mesh.vertexCapacity, size);
    safe_expand(mesh.indices, mesh.indexCount, mesh.indexCapacity, size * 6);

    float *heights = map->heights[chunk].data;

    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            float nx = 0.0f, ny = 1.0f, nz = 0.0f;

            float height = heights[y * CHUNK_DIM + x];

            if (x > 0 && x < CHUNK_DIM - 1 && y > 0 && y < CHUNK_DIM - 1) {
                float up    = heights[(y - 1) * CHUNK_DIM + x] - height;
                float down  = heights[(y + 1) * CHUNK_DIM + x] - height;
                float left  = heights[y * CHUNK_DIM + x - 1] - height;
                float right = heights[y * CHUNK_DIM + x + 1] - height;

                float upLeft = heights[(y - 1) * CHUNK_DIM + x - 1] - height;
                float upRight = heights[(y - 1) * CHUNK_DIM + x + 1] - height;
                float downLeft = heights[(y + 1) * CHUNK_DIM + x - 1] - height;
                float downRight = heights[(y + 1) * CHUNK_DIM + x + 1] - height;

                nx = ny = nz = 0.0f;

                float vecs[][3] = {
                    { 0.0f, up,       -1.0f },
                    {-1.0f, upLeft,   -1.0f },
                    {-1.0f, left,      0.0f },
                    {-1.0f, downLeft,  1.0f },
                    { 0.0f, down,      1.0f },
                    { 1.0f, downRight, 1.0f },
                    { 1.0f, right,     0.0f },
                    { 1.0f, upRight,  -1.0f },
                };

                for (int i = 0; i < length(vecs); ++i) {
                    float norm[3];
                    float *a = vecs[i];
                    float *b = vecs[(i + 1) % length(vecs)];
                    cross(norm, a, b);

                    float invLen = 1.0f / vecLen(norm);
                    norm[0] *= invLen;
                    norm[1] *= invLen;
                    norm[2] *= invLen;

                    float weight = acosf(dot(a, b) / (vecLen(a) * vecLen(b))) / (2 * M_PI);

                    nx += norm[0] * weight;
                    ny += norm[1] * weight;
                    nz += norm[2] * weight;
                }

            }

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

    /* indices */
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

