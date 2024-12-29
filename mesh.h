#pragma once

#include "texture.h"

typedef struct {
    float x;
    float y;
    float z;
    float u;
    float v;
} MeshPoint;

typedef struct {
    int a, b, c;
} MeshTriangle;

typedef struct {
    int texture_count;
    Texture *textures;
    int point_count;
    MeshPoint *points;
    int triangle_count;
    MeshTriangle *triangles;
} Mesh;

BOOL Mesh_LoadFromPathologicFormat(Mesh *mesh, const char *path);
