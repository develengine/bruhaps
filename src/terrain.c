#include "terrain.h"

#include "utils.h"


ChunkMesh constructChunkMesh(Map *map, int chunk)
{
    ChunkMesh mesh = { 0 };

    const int size = CHUNK_DIM * CHUNK_DIM;
    safe_expand(mesh.vertices, mesh.vertexCount, mesh.vertexCapacity, size);
    safe_expand(mesh.indices, mesh.indexCount, mesh.indexCapacity, size * 6);

    uint8_t *heights = map->chunks[chunk].data;

    /* vertices */
    for (int y = 0; y < CHUNK_DIM; ++y) {
        for (int x = 0; x < CHUNK_DIM; ++x) {
            float nx = 1.0f, ny = 1.0f, nz = 1.0f;

            float height = (heights[y * CHUNK_DIM + x] / 255.0f) * CHUNK_HEIGHT;

            /* FIXME shit normals */
            if (x > 0 && x < CHUNK_DIM - 1 && y > 0 && y < CHUNK_DIM - 1) {
                float up    = (heights[(y - 1) * CHUNK_DIM + x] / 255.0f) * CHUNK_HEIGHT;
                float down  = (heights[(y + 1) * CHUNK_DIM + x] / 255.0f) * CHUNK_HEIGHT;
                float left  = (heights[y * CHUNK_DIM + x - 1] / 255.0f) * CHUNK_HEIGHT;
                float right = (heights[y * CHUNK_DIM + x + 1] / 255.0f) * CHUNK_HEIGHT;
                nx = ((left - height) - (right - height)) * 0.5f;
                ny = 1.0f;
                nz = ((up - height) - (down - height)) * 0.5f;
                float invLen = 1.0f / sqrtf(nx * nx + ny * ny + nz * nz);
                nx *= invLen;
                ny *= invLen;
                nz *= invLen;
            }

            Vertex vertex = {
                .positions = {
                    (float)x * CHUNK_TILE_DIM,
                    height,
                    (float)y * CHUNK_TILE_DIM
                },
                .normals = { nx, ny, nz }
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

