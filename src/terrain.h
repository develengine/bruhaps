#ifndef TERRAIN_H
#define TERRAIN_H

#include "res.h"

#include <stdint.h>
#include <stdlib.h>

#define CHUNK_DIM 16
#define CHUNK_TILE_DIM 1.0f
#define CHUNK_HEIGHT 32.0f

#define MAX_MAP_DIM 32
#define NO_CHUNK    -1
#define NO_TILE     666.f


typedef struct
{
    float data[CHUNK_DIM * CHUNK_DIM];
} ChunkHeights;


typedef struct
{
    uint8_t viewID;
    uint8_t trans;
    uint8_t x, y;
} TileTexture;


typedef struct
{
    float x, y, w, h;
    int wn, hn;
} AtlasView;


typedef struct
{
    TileTexture data[CHUNK_DIM * CHUNK_DIM];
} ChunkTextures;


typedef struct
{
    int chunkMap[MAX_MAP_DIM * MAX_MAP_DIM];
    ChunkHeights *heights;
    ChunkTextures *textures;
    int chunkCount, chunkCapacity;
} Terrain;


typedef struct
{
    Vertex *vertices;
    int vertexCount, vertexCapacity;

    unsigned *indices;
    int indexCount, indexCapacity;
} ChunkMesh;


typedef struct
{
    unsigned vao;
    unsigned vbo;
    unsigned ebo;
    unsigned vertexCount;
    unsigned indexCount;
} ChunkObject;


ChunkObject createChunkObject(void);


static inline Model chunkMeshToModel(ChunkMesh mesh)
{
    Model model = {
        .vertexCount = mesh.vertexCount,
        .indexCount  = mesh.indexCount,
        .vertices    = mesh.vertices,
        .indices     = mesh.indices
    };

    return model;
}

static inline void freeChunkMesh(ChunkMesh mesh)
{
    free(mesh.vertices);
    free(mesh.indices);
}

void clearChunks(Terrain *terrain);

float atTerrainHeight(const Terrain *terrain, int x, int z);

void constructChunkMesh(
        ChunkMesh *mesh,
        const Terrain *terrain,
        const AtlasView *atlasViews,
        int cx,
        int cz
);

#endif
