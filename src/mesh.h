#pragma once

#include "texture.h"

#define MESH_VERTICES_TYPE_FLOAT 0
#define MESH_VERTICES_TYPE_SHORT 1

typedef struct {
    struct { short x, y, z; } pos_short; // only if vertices_type == 1
    struct { float x, y, z; } pos;
    struct { short x, y; } norm_short; // packed normal (two xy components, z must be restored)
    struct { float x, y, z; } norm;
    float u;
    float v;
} MeshPoint;

typedef struct {
    int a, b, c;
} MeshTriangle;

typedef struct {
    float x, y, z, radius;
} Sphere;

typedef struct {
    float x, y, z; // pos
    float orientation[9]; // matrix 3x3
    float extents[3]; // half-size along each axis
} OBB;

typedef struct {
    Sphere sphere;
    OBB obb;
} SphereWithOBB;

typedef struct {
    int unknown_4;
    int unknown_5;
    int unknown_array_length_1;
    int unknown_array_length_2;
    int unknown_array_length_3;
    int unknown_array_length_4;
    float unknown_13;
} SubmeshHeaderUnknownBlockSubblock;

typedef struct {
    int unknown_2;
    float unknown_15floats[15];
    int subblocks_count;
    SubmeshHeaderUnknownBlockSubblock *subblocks;
} SubmeshHeaderUnknownBlock;

typedef struct {
    int id; // probably just an ID (note that submeshes are not sorted by this value)
    int material_type; // probably bitflags, affects the header structure

    char unknown_3;
    float unknown_4[6];

    unsigned char texture_name_length;
    char texture_name[255];

    char texture_path[MAX_PATH];
    Texture texture;

    int point_count;
    int indices_count;

    SphereWithOBB sphere_with_obb;

    MeshPoint *points;
    MeshTriangle *indices;

    float world_x; // world coordinates
    float world_y;
    float world_z;

    BYTE vertices_type;

    int unknown_blocks_count;
    SubmeshHeaderUnknownBlock *unknown_blocks;
} SubmeshHeader;

typedef struct {
    int submesh_count;
    SubmeshHeader *submeshes;
} Mesh;

BOOL Mesh_LoadFromPathologicFormat(Mesh *mesh, const char *path);
