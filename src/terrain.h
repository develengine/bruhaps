#ifndef TERRAIN_H
#define TERRAIN_H

#include "res.h"

#include <stdint.h>
#include <stdlib.h>

#define CHUNK_DIM 16
#define CHUNK_TILE_DIM 1.0f
#define CHUNK_HEIGHT 32.0f

typedef struct
{
    float data[CHUNK_DIM * CHUNK_DIM];
} ChunkHeights;


typedef struct
{
    uint8_t id;
    uint8_t rot;
    uint8_t x, y;
} TileTexture;


typedef struct
{
    float x, y, w, h;
    float wn, hn;
} MapTexView;


typedef struct
{
    TileTexture data[CHUNK_DIM * CHUNK_DIM];
} ChunkTextures;


#define MAX_MAP_DIM 64
#define NO_CHUNK -1
#define NO_TILE 666.f

typedef struct
{
    int chunkMap[MAX_MAP_DIM * MAX_MAP_DIM];
    ChunkHeights *heights;
    ChunkTextures *textures;
    int chunkCount, chunkCapacity;
} Map;


typedef struct
{
    Vertex *vertices;
    int vertexCount, vertexCapacity;

    unsigned *indices;
    int indexCount, indexCapacity;
} ChunkMesh;

inline void freeChunkMesh(ChunkMesh mesh)
{
    free(mesh.vertices);
    free(mesh.indices);
}


float atMapHeight(Map *map, int x, int z);

ChunkMesh constructChunkMesh(Map *map, int cx, int cz);

#endif
