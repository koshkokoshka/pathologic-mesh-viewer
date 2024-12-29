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
    Texture texture;
    int point_count;
    MeshPoint *points;
    int triangle_count;
    MeshTriangle *triangles;
} MeshSubmesh;

typedef struct {
    int submesh_count;
    MeshSubmesh *submeshes;
} Mesh;

BOOL Mesh_LoadFromPathologicFormat(Mesh *mesh, const char *path);
