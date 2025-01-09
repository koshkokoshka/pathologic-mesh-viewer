#include "mesh.h"

#include <shlwapi.h>
#include <stdint.h>

#include "draw.h"

//
// Notes:
// - In .mesh file geometry data grouped by unique material (usually it's a diffuse texture)
//   and for simplicity I called every block of grouped geometry data a "submesh"
//
// material_type=33 - isobor_lampa.mesh
// material_type=33 - icot_eva_Kerosinka.mesh
// material_type=17 - inv_grass_white_plet.mesh
//

#define ReadInt(file, result) ( ReadFile(file, result, sizeof(int), NULL, NULL) )

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

    int point_count;
    int indices_count;

    int unknown_blocks_count;
    SubmeshHeaderUnknownBlock *unknown_blocks;

} SubmeshHeader;

BOOL Mesh_ReadSubmeshHeader(OUT SubmeshHeader *submesh_header, HANDLE file, DWORD file_size)
{
    // Submesh ID
    if (!ReadFile(file, &submesh_header->id, sizeof(int), NULL, NULL)) { return FALSE; }

    // Material type
    if (!ReadFile(file, &submesh_header->material_type, sizeof(int), NULL, NULL)) { return FALSE; }
    if (submesh_header->material_type == 0) {
        // ??? unknown byte
        if (!ReadFile(file, &submesh_header->unknown_3, sizeof(char), NULL, NULL)) { return FALSE; }
        // ??? unknown 6 floats
        if (!ReadFile(file, &submesh_header->unknown_4, sizeof(float) * 6, NULL, NULL)) { return FALSE; }
    } else {
        if (submesh_header->material_type == 1 || submesh_header->material_type == 17) {
            // ??? unknown byte
            if (!ReadFile(file, &submesh_header->unknown_3, sizeof(char), NULL, NULL)) { return FALSE; }
        }

        // Read texture name
        if (!ReadFile(file, &submesh_header->texture_name_length, sizeof(char), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, submesh_header->texture_name, submesh_header->texture_name_length, NULL, NULL)) { return FALSE; }
        submesh_header->texture_name[submesh_header->texture_name_length] = '\0';

        // ???
        if (submesh_header->material_type == 1 || submesh_header->material_type == 33) {
            char unknown_4;
            if (!ReadFile(file, &unknown_4, sizeof(char), NULL, NULL)) { return FALSE; }
        }
        if (submesh_header->material_type == 5) {
            char unknown_4;
            if (!ReadFile(file, &unknown_4, sizeof(char), NULL, NULL)) { return FALSE; }
            float unknown_4floats[4];
            if (!ReadFile(file, unknown_4floats, sizeof(unknown_4floats), NULL, NULL)) { return FALSE; }
        }
    }

    // Read vertices and indices count
    if (!ReadFile(file, &submesh_header->point_count, sizeof(int), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &submesh_header->indices_count, sizeof(int), NULL, NULL)) { return FALSE; }

    // Sanity checks
    if ((submesh_header->point_count * sizeof(int)) > file_size) { return FALSE; }
    if ((submesh_header->indices_count * sizeof(int)) > file_size) { return FALSE; }

    // ??? probably it's a sphere xyz coordinates and radius followed by OBB (oriented bounding box) collision data
    float unknown_19floats[19];
    if (!ReadFile(file, unknown_19floats, sizeof(float) * 19, NULL, NULL)) { return FALSE; }

    // ??? unknown blocks
    if (submesh_header->material_type == 0 || submesh_header->material_type == 5 || submesh_header->material_type == 1 || submesh_header->material_type == 17) {
        int unknown_blocks_count;
        if (!ReadFile(file, &unknown_blocks_count, sizeof(int), NULL, NULL)) { return FALSE; }
        // Sanity check
        if ((unknown_blocks_count * sizeof(SubmeshHeaderUnknownBlock)) > file_size) { return FALSE; }

        submesh_header->unknown_blocks_count = unknown_blocks_count;
        if (unknown_blocks_count > 0) {
            submesh_header->unknown_blocks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeaderUnknownBlock) * unknown_blocks_count);
            for (int j = 0; j < unknown_blocks_count; ++j) {
                SubmeshHeaderUnknownBlock *unknown_block = &submesh_header->unknown_blocks[j];

                if (!ReadFile(file, &unknown_block->unknown_2, sizeof(int), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, unknown_block->unknown_15floats, sizeof(float) * 15, NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &unknown_block->subblocks_count, sizeof(int), NULL, NULL)) { return FALSE; }
                if (unknown_block->subblocks_count != 0) {
                    unknown_block->subblocks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeaderUnknownBlockSubblock) * unknown_block->subblocks_count);

                    for (int k = 0; k < unknown_block->subblocks_count; ++k) {
                        SubmeshHeaderUnknownBlockSubblock *subblock = &unknown_block->subblocks[k];

                        if (!ReadFile(file, &subblock->unknown_4, sizeof(int), NULL, NULL)) { return FALSE; }
                        if (!ReadFile(file, &subblock->unknown_5, sizeof(int), NULL, NULL)) { return FALSE; }
                        if (!ReadFile(file, &subblock->unknown_array_length_1, sizeof(int), NULL, NULL)) { return FALSE; }
                        SetFilePointer(file, sizeof(char) * subblock->unknown_array_length_1, NULL, FILE_CURRENT); // skip
                        if (!ReadFile(file, &subblock->unknown_array_length_2, sizeof(int), NULL, NULL)) { return FALSE; }
                        SetFilePointer(file, sizeof(char) * subblock->unknown_array_length_2, NULL, FILE_CURRENT); // skip
                        if (!ReadFile(file, &subblock->unknown_array_length_3, sizeof(int), NULL, NULL)) { return FALSE; }
                        if (!ReadFile(file, &subblock->unknown_array_length_4, sizeof(int), NULL, NULL)) { return FALSE; }
                        SetFilePointer(file, sizeof(short) * 4 * subblock->unknown_array_length_4, NULL, FILE_CURRENT); // skip
                        if (!ReadFile(file, &subblock->unknown_13, sizeof(float), NULL, NULL)) { return FALSE; }
                    }
                }
            }
        }
    }

    return TRUE;
}

BOOL FindTexturePath(char *output, const char *texture_name, const char *file_path)
{
    strcpy(output, file_path); // copy full .mesh file path
    if (!PathRemoveFileSpec(output)) { return FALSE; } // remove file name
    if (!PathRemoveFileSpec(output)) { return FALSE; } // go up dir
    if (PathCombine(output, output, "Textures") == NULL) { return FALSE; } // go to the "Textures" directory
    if (PathCombine(output, output, texture_name) == NULL) { return FALSE; } // add texture name
    return TRUE;
}

BOOL Mesh_ReadMethod6(OUT Mesh *mesh, HANDLE file, DWORD file_size, const char *file_path)
{
    int unknown_1 = 0;
    if (!ReadFile(file, &unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }

    int submesh_count = 0;
    if (!ReadFile(file, &submesh_count, sizeof(int), NULL, NULL)) { return FALSE; }

    // Allocate memory to store submeshes
    mesh->submesh_count = submesh_count;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshSubmesh) * submesh_count);

    // Read submesh headers
    SubmeshHeader *submesh_headers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeader) * submesh_count);
    for (int i = 0; i < submesh_count; ++i) {
        SubmeshHeader *submesh_header = &submesh_headers[i];
        if (!Mesh_ReadSubmeshHeader(submesh_header, file, file_size)) { // read every submesh header block
            return FALSE;
        }
    }

    // Load all textures specified in submesh headers
    for (int i = 0; i < submesh_count; ++i) {
        SubmeshHeader *header = &submesh_headers[i];

        if (header->material_type != 0) {

            MeshSubmesh *submesh = &mesh->submeshes[i];

            // Find texture path
            if (!FindTexturePath(submesh->texture_path, header->texture_name, file_path)) {
                return FALSE;
            }
            // Load texture
            if (!Texture_Load(&submesh->texture, submesh->texture_path)) {
                submesh->texture = fallback_texture; // use fallback texture
            }
        }
    }

    // Read blocks data
    for (int i = 0; i < submesh_count; ++i) {
        SubmeshHeader *submesh_header = &submesh_headers[i];
        MeshSubmesh *submesh = &mesh->submeshes[i];

        char vertices_type;
        if (!ReadFile(file, &vertices_type, sizeof(char), NULL, NULL)) { return FALSE; }

        if (vertices_type != 0) {
            // ??? (probably some coordinates)
            float unknown_float1;
            if (!ReadFile(file, &unknown_float1, sizeof(float), NULL, NULL)) { return FALSE; }
            float unknown_float2;
            if (!ReadFile(file, &unknown_float2, sizeof(float), NULL, NULL)) { return FALSE; }
            float unknown_float3;
            if (!ReadFile(file, &unknown_float3, sizeof(float), NULL, NULL)) { return FALSE; }
        }

        // Read points
        submesh->point_count = submesh_header->point_count;
        submesh->points = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshPoint) * submesh->point_count);
        for (int j = 0; j < submesh_header->point_count; ++j) {
            MeshPoint *point = &submesh->points[j];

            if (vertices_type == 0) {
                // FLOAT
                float x, y, z;
                if (!ReadFile(file, &x, sizeof(float), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &y, sizeof(float), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &z, sizeof(float), NULL, NULL)) { return FALSE; }
                point->x = x;
                point->y = y;
                point->z = z;
            } else {
                // SHORT
                short x, y, z;
                if (!ReadFile(file, &x, sizeof(short), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &y, sizeof(short), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &z, sizeof(short), NULL, NULL)) { return FALSE; }
                point->x = (float)x / INT16_MAX; // unpack int16 coordinates
                point->y = (float)y / INT16_MAX;
                point->z = (float)z / INT16_MAX;
            }

            if (submesh_header->material_type == 0) {
                int unknown_1;
                if (!ReadFile(file, &unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
            }
            if (submesh_header->material_type == 1) {
                int unknown_1;
                if (!ReadFile(file, &unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
                float u, v;
                if (!ReadFile(file, &u, sizeof(float), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &v, sizeof(float), NULL, NULL)) { return FALSE; }
                point->u = u;
                point->v = v;
            }
            if (submesh_header->material_type == 5) {
                int unknown_1;
                if (!ReadFile(file, &unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
                float u, v;
                if (!ReadFile(file, &u, sizeof(float), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &v, sizeof(float), NULL, NULL)) { return FALSE; }
                point->u = u;
                point->v = v;
            }
            if (submesh_header->material_type == 17) {
                int unknown_1;
                if (!ReadFile(file, &unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
                float u, v;
                if (!ReadFile(file, &u, sizeof(float), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &v, sizeof(float), NULL, NULL)) { return FALSE; }
                point->u = u;
                point->v = v;
            }
            if (submesh_header->material_type == 33) {
                float u, v;
                if (!ReadFile(file, &u, sizeof(float), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &v, sizeof(float), NULL, NULL)) { return FALSE; }
                point->u = u;
                point->v = v;
            }
            if (submesh_header->unknown_3 == 1) {
                int unknown_1;
                if (!ReadFile(file, &unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
            }
        }

        // Read triangles
        submesh->triangle_count = submesh_header->indices_count;
        submesh->triangles = HeapAlloc(GetProcessHeap(), 0, sizeof(MeshTriangle) * submesh->triangle_count);
        for (int j = 0; j < submesh_header->indices_count; ++j) {
            MeshTriangle *triangle = &submesh->triangles[j];

            // Read indices
            unsigned short v1, v2, v3;
            if (!ReadFile(file, &v1, sizeof(unsigned short), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &v2, sizeof(unsigned short), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &v3, sizeof(unsigned short), NULL, NULL)) { return FALSE; }

            // Validate indices bounds
            if (v1 < 0 || v1 >= submesh->point_count) { return FALSE; }
            if (v2 < 0 || v2 >= submesh->point_count) { return FALSE; }
            if (v3 < 0 || v3 >= submesh->point_count) { return FALSE; }

            triangle->a = v1;
            triangle->b = v2;
            triangle->c = v3;
        }

        // ???
        for (int i2 = 0; i2 < submesh_header->unknown_blocks_count; ++i2) {
            SubmeshHeaderUnknownBlock *unknown_block = &submesh_header->unknown_blocks[i2];

            for (int i3 = 0; i3 < unknown_block->subblocks_count; ++i3) {
                SubmeshHeaderUnknownBlockSubblock *subblock = &unknown_block->subblocks[i3];

                for (int i4 = 0; i4 < subblock->unknown_array_length_3; ++i4) {
                    unsigned short unknown_short;
                    if (!ReadFile(file, &unknown_short, sizeof(short), NULL, NULL)) { return FALSE; }

                    float unknown_float4;
                    if (!ReadFile(file, &unknown_float4, sizeof(float), NULL, NULL)) { return FALSE; }
                    float unknown_float5;
                    if (!ReadFile(file, &unknown_float5, sizeof(float), NULL, NULL)) { return FALSE; }

                    if (submesh_header->unknown_3 == 1 || submesh_header->unknown_3 == 17) {
                        short unknown_short1;
                        if (!ReadFile(file, &unknown_short1, sizeof(short), NULL, NULL)) { return FALSE; }
                        short unknown_short2;
                        if (!ReadFile(file, &unknown_short2, sizeof(short), NULL, NULL)) { return FALSE; }
                    }
                }
            }
        }
    }

    return TRUE;
}

/**
 * Related to Method 4
 */
typedef struct {
    struct {
        BYTE unknown_1;
        float unknown_2;
        float unknown_3;
        float unknown_4;
        float unknown_5;
    } unknown_1;
    int index;
    struct {
        int unknown_1_count;
        struct {
            int unknown_1;
            int unknown_2;
        } unknown_1; // unknown_1[unknown_1_count]
        float unknown_2;
        float unknown_3;
        float unknown_4;
    } unknown_2;
    int unknown_3_length;
    char *unknown_3;
    int unknown_4_length;
    int *unknown_4;
    int unknown_5_count;
    struct {
        float unknown_1;
        float unknown_2;
        float unknown_3;
        float unknown_4;
        float unknown_5;
        float unknown_6;
        float unknown_7;
        float unknown_8;
        float unknown_9;
    } unknown_5_1; // unknown_5_1[unknown_5_count]
    byte unknown_5_2; // unknown_5_2[unknown_5_count]
} Block1;

/**
 * Related to Method 4
 */
typedef struct {
    int triangles_count_1;
    int unknown_1; // usually equals to "1", purpose is unknown
    BYTE texture_name_length;
    char texture_name[MAX_PATH]; // texture_name[texture_name_length]
    int triangles_count_2;
    BYTE vertices_type; // 0 - float, 1 - short
    float unknown_4; // probably world xyz coordinates
    float unknown_5;
    float unknown_6;
    struct {
        union {
            struct { float x, y, z; } pos_float;
            struct { short x, y, z; } pos_short;
        };
        char unknown_1[4];
        float u, v;
    } *vertices; // vertices[triangles_count_2 * 3]
} Block3;

/**
 * Related to Method 4
 */
typedef struct {
    int unknown_1;
    int unknown_2;
    int unknown_3_length;
    char *unknown_3;
    int unknown_4_length;
    char *unknown_4;
    int unknown_5_count;
    struct {
        short unknown_1;
        float unknown_2;
        float unknown_3;
        float unknown_4;
        float unknown_5;
    } unknown_5;
    float unknown_6;
    struct {
        int unknown_1;
        int unknown_2;
    } unknown_7; // unknown_7[triangles_count]
} Block4;

/**
 * Method 4:
 *
 * Examples
 * - ihouse_kabak.mesh
 * - warehouse1.mesh
 * - theater_indoor.mesh
 */
BOOL Mesh_ReadMethod4(OUT Mesh *mesh, HANDLE file, DWORD file_size, const char *file_path)
{
    //
    // Read "Block 1" data (purpose of this block is unclean, perhaps objects, particles, or lighting data)
    //

    // Read block count
    int block1_count;
    if (!ReadFile(file, &block1_count, sizeof(int), NULL, NULL)) { return FALSE; }
    if (block1_count * sizeof(Block1) > file_size) { return FALSE; } // overflow check

    // Read blocks data
    Block1 block1;
    for (int i1 = 0; i1 < block1_count; ++i1) {

        // Read "block1.unknown_1" block until "block1.unknown_1.unknown_1" is not 0
        while (1) {
            if (!ReadFile(file, &block1.unknown_1.unknown_1, sizeof(BYTE), NULL, NULL)) { return FALSE; }
            if (block1.unknown_1.unknown_1 == 0) {
                break; // done reading
            }

            if (!ReadFile(file, &block1.unknown_1.unknown_2, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_1.unknown_3, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_1.unknown_4, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_1.unknown_5, sizeof(float), NULL, NULL)) { return FALSE; }
        }

        // Read current block index
        if (!ReadFile(file, &block1.index, sizeof(int), NULL, NULL)) { return FALSE; } // read current block index
        // Safety check: block index specified in the file should match the currently reading block index
        if (block1.index != i1) {
            return FALSE; // indices do not match
        }

        // Read "block1.unknown_2" block
        if (!ReadFile(file, &block1.unknown_2.unknown_1_count, sizeof(int), NULL, NULL)) { return FALSE; }
        for (int i2 = 0; i2 < block1.unknown_2.unknown_1_count; ++i2) {
            if (!ReadFile(file, &block1.unknown_2.unknown_1.unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_2.unknown_1.unknown_2, sizeof(int), NULL, NULL)) { return FALSE; }
        }
        if (!ReadFile(file, &block1.unknown_2.unknown_2, sizeof(float), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &block1.unknown_2.unknown_3, sizeof(float), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &block1.unknown_2.unknown_4, sizeof(float), NULL, NULL)) { return FALSE; }

        if (!ReadFile(file, &block1.unknown_3_length, sizeof(int), NULL, NULL)) { return FALSE; }
        SetFilePointer(file, block1.unknown_3_length * sizeof(block1.unknown_3[0]), NULL, FILE_CURRENT); // skip unknown data
        if (!ReadFile(file, &block1.unknown_4_length, sizeof(int), NULL, NULL)) { return FALSE; }
        SetFilePointer(file, block1.unknown_4_length * sizeof(block1.unknown_4[0]), NULL, FILE_CURRENT); // skip unknown data

        // Read "block1.unknown_5" block
        if (!ReadFile(file, &block1.unknown_5_count, sizeof(int), NULL, NULL)) { return FALSE; }
        if (block1.unknown_5_count * sizeof(block1.unknown_5_1) > file_size) { return FALSE; } // overflow check
        for (int i2 = 0; i2 < block1.unknown_5_count; ++i2) {
            if (!ReadFile(file, &block1.unknown_5_1.unknown_1, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_5_1.unknown_2, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_5_1.unknown_3, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_5_1.unknown_4, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_5_1.unknown_5, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_5_1.unknown_6, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_5_1.unknown_7, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_5_1.unknown_8, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block1.unknown_5_1.unknown_9, sizeof(float), NULL, NULL)) { return FALSE; }
        }
        for (int i2 = 0; i2 < block1.unknown_5_count; ++i2) {
            if (!ReadFile(file, &block1.unknown_5_2, sizeof(BYTE), NULL, NULL)) { return FALSE; }
        }
    }

    //
    // Read "Block 2" data
    //
    int block2_count;
    if (!ReadFile(file, &block2_count, sizeof(int), NULL, NULL)) { return FALSE; }

    int unknown_1; // unknown values that somehow corresponding with "Block 2" data
    for (int i = 0; i < block2_count; ++i) {
        if (!ReadFile(file, &unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
    }

    //
    // Read "Block 2" data
    //
    int block3_count;
    if (!ReadFile(file, &block3_count, sizeof(int), NULL, NULL)) { return FALSE; }
    if (block3_count * sizeof(Block3) > file_size) { return FALSE; } // overflow check

    Block3 *blocks3 = HeapAlloc(GetProcessHeap(), 0, block3_count * sizeof(Block3));
    for (int i1 = 0; i1 < block3_count; ++i1) {
        Block3 *block3 = &blocks3[i1];

        if (!ReadFile(file, &block3->triangles_count_1, sizeof(int), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &block3->unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }

        if (!ReadFile(file, &block3->texture_name_length, sizeof(BYTE), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &block3->texture_name, block3->texture_name_length, NULL, NULL)) { return FALSE; }
        block3->texture_name[block3->texture_name_length] = 0;

        if (!ReadFile(file, &block3->triangles_count_2, sizeof(int), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &block3->vertices_type, sizeof(BYTE), NULL, NULL)) { return FALSE; }
        if (block3->vertices_type == 1) {
            if (!ReadFile(file, &block3->unknown_4, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block3->unknown_5, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block3->unknown_6, sizeof(float), NULL, NULL)) { return FALSE; }
        }

        int vertices_count = block3->triangles_count_2 * 3;
        if (vertices_count * sizeof(block3->vertices[0]) > file_size) { return FALSE; } // overflow check
        block3->vertices = HeapAlloc(GetProcessHeap(), 0, vertices_count * sizeof(block3->vertices[0]));
        if (block3->vertices == NULL) { return FALSE; }
        for (int i2 = 0; i2 < vertices_count; ++i2) {
            if (block3->vertices_type == 1) {
                if (!ReadFile(file, &block3->vertices[i2].pos_short.x, sizeof(short), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &block3->vertices[i2].pos_short.y, sizeof(short), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &block3->vertices[i2].pos_short.z, sizeof(short), NULL, NULL)) { return FALSE; }
            } else {
                if (!ReadFile(file, &block3->vertices[i2].pos_float.x, sizeof(float), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &block3->vertices[i2].pos_float.y, sizeof(float), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &block3->vertices[i2].pos_float.z, sizeof(float), NULL, NULL)) { return FALSE; }
            }
            if (!ReadFile(file, &block3->vertices[i2].unknown_1, sizeof(block3->vertices[i2].unknown_1), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block3->vertices[i2].u, sizeof(float), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &block3->vertices[i2].v, sizeof(float), NULL, NULL)) { return FALSE; }
        }

        for (int i2 = 0; i2 < block2_count; ++i2) {
            int block4_count;
            if (!ReadFile(file, &block4_count, sizeof(int), NULL, NULL)) { return FALSE; }
            if (block4_count <= 0) {
                continue; // read the next data only if "block4_count" value greater than zero
            }

            if (block4_count * sizeof(Block4) > file_size) { return FALSE; } // overflow check

            Block4 block4;
            for (int i3 = 0; i3 < block4_count; ++i3) {
                if (!ReadFile(file, &block4.unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &block4.unknown_2, sizeof(int), NULL, NULL)) { return FALSE; }

                if (!ReadFile(file, &block4.unknown_3_length, sizeof(int), NULL, NULL)) { return FALSE; }
                SetFilePointer(file, block4.unknown_3_length, NULL, FILE_CURRENT); // skip unknown data

                if (!ReadFile(file, &block4.unknown_4_length, sizeof(int), NULL, NULL)) { return FALSE; }
                SetFilePointer(file, block4.unknown_4_length, NULL, FILE_CURRENT); // skip unknown data

                if (!ReadFile(file, &block4.unknown_5_count, sizeof(int), NULL, NULL)) { return FALSE; } // note that this value is multiplied by 3 for the following loop
                for (int i4 = 0; i4 < block4.unknown_5_count * 3; ++i4) {
                    if (!ReadFile(file, &block4.unknown_5.unknown_1, sizeof(short), NULL, NULL)) { return FALSE; }
                    if (!ReadFile(file, &block4.unknown_5.unknown_2, sizeof(float), NULL, NULL)) { return FALSE; }
                    if (!ReadFile(file, &block4.unknown_5.unknown_3, sizeof(float), NULL, NULL)) { return FALSE; }
                    if (!ReadFile(file, &block4.unknown_5.unknown_4, sizeof(float), NULL, NULL)) { return FALSE; }
                    if (!ReadFile(file, &block4.unknown_5.unknown_5, sizeof(float), NULL, NULL)) { return FALSE; }
                }
                if (!ReadFile(file, &block4.unknown_6, sizeof(float), NULL, NULL)) { return FALSE; }
            }

            for (int i3 = 0; i3 < block3->triangles_count_2; ++i3) {
                if (!ReadFile(file, &block4.unknown_7.unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &block4.unknown_7.unknown_2, sizeof(int), NULL, NULL)) { return FALSE; }
            }
        }
    }

    //
    // Unpack read data
    //
    mesh->submesh_count = block3_count;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshSubmesh) * mesh->submesh_count);
    if (mesh->submeshes == NULL) {
        return FALSE;
    }

    // Load textures first
    for (int i1 = 0; i1 < mesh->submesh_count; ++i1) {
        MeshSubmesh *submesh = &mesh->submeshes[i1];
        Block3 *block3 = &blocks3[i1];

        if (   !FindTexturePath(submesh->texture_path, block3->texture_name, file_path)
            || !Texture_Load(&submesh->texture, submesh->texture_path)) {
            submesh->texture = fallback_texture; // use fallback texture
        }
    }

    for (int i1 = 0; i1 < mesh->submesh_count; ++i1) {
        MeshSubmesh *submesh = &mesh->submeshes[i1];
        Block3 *block3 = &blocks3[i1];

        submesh->point_count = block3->triangles_count_2 * 3;
        submesh->points = HeapAlloc(GetProcessHeap(), 0, submesh->point_count * sizeof(MeshPoint));
        if (submesh->points == NULL) {
            return FALSE;
        }
        for (int i2 = 0; i2 < submesh->point_count; ++i2) {
            if (block3->vertices_type == 1) {
                submesh->points[i2].x = (float)block3->vertices[i2].pos_short.x / INT16_MAX; // unpack coordinates
                submesh->points[i2].y = (float)block3->vertices[i2].pos_short.y / INT16_MAX;
                submesh->points[i2].z = (float)block3->vertices[i2].pos_short.z / INT16_MAX;
            } else {
                submesh->points[i2].x = block3->vertices[i2].pos_float.x;
                submesh->points[i2].y = block3->vertices[i2].pos_float.y;
                submesh->points[i2].z = block3->vertices[i2].pos_float.z;
            }
            submesh->points[i2].u = block3->vertices[i2].u;
            submesh->points[i2].v = block3->vertices[i2].v;
        }

        submesh->triangle_count = block3->triangles_count_2;
        submesh->triangles = HeapAlloc(GetProcessHeap(), 0, submesh->triangle_count * sizeof(MeshTriangle));
        if (submesh->triangles == NULL) {
            return FALSE;
        }
        for (int i2 = 0; i2 < submesh->triangle_count; ++i2) {
            submesh->triangles[i2].a = ((i2 * 3) + 0) % submesh->point_count; // calculate indices
            submesh->triangles[i2].b = ((i2 * 3) + 1) % submesh->point_count;
            submesh->triangles[i2].c = ((i2 * 3) + 2) % submesh->point_count;
        }
    }

    return TRUE;
}

/**
 * Method 1: vertices data first, single material, no uv, no texture
 *
 * Examples:
 * - pond_water.mesh
 *
 * struct {
 *     int vertices_count;
 *     struct {
 *         float x, y, z;
 *     } vertices[vertices_count];
 *     int indices_count;
 *     struct {
 *         short a, b, c;
 *     } indices[indices_count];
 *     float unknown_1[3];
 *     struct {
 *         float unknown_1;
 *         float unknown_2;
 *         float unknown_3;
 *     } unknown_2[vertices_count];
 * }
 */
BOOL Mesh_ReadMethod1(OUT Mesh *mesh, HANDLE file, DWORD file_size, const char *file_path)
{
    //
    // Read file
    //
    struct { float x, y, z; } *vertices;
    struct { short a, b, c; } *indices;

    // Read vertices
    int vertices_count = 0;
    if (!ReadFile(file, &vertices_count, sizeof(int), NULL, NULL)) { return FALSE; }

    DWORD vertices_size = vertices_count * sizeof(vertices[0]);
    if (vertices_size > file_size) { return FALSE; } // overflow check

    vertices = HeapAlloc(GetProcessHeap(), 0, vertices_size);
    for (int i = 0; i < vertices_count; ++i) {
        if (!ReadFile(file, &vertices[i].x, sizeof(float), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].y, sizeof(float), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].z, sizeof(float), NULL, NULL)) { return FALSE; }
    }

    // Read indices
    int indices_count = 0;
    if (!ReadFile(file, &indices_count, sizeof(int), NULL, NULL)) { return FALSE; }

    DWORD indices_size = indices_count * sizeof(indices[0]);
    if (indices_size > file_size) { return FALSE; } // overflow check

    indices = HeapAlloc(GetProcessHeap(), 0, indices_size);
    for (int i = 0; i < indices_count; ++i) {
        if (!ReadFile(file, &indices[i].a, sizeof(short), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &indices[i].b, sizeof(short), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &indices[i].c, sizeof(short), NULL, NULL)) { return FALSE; }
    }

    // Validate indices bounds
    for (int i = 0; i < indices_count; ++i) {
        if (indices[i].a < 0 || indices[i].a >= vertices_count) { return FALSE; }
        if (indices[i].b < 0 || indices[i].b >= vertices_count) { return FALSE; }
        if (indices[i].c < 0 || indices[i].c >= vertices_count) { return FALSE; }
    }

    float unknown_1[3];
    if (!ReadFile(file, unknown_1, sizeof(unknown_1), NULL, NULL)) { return FALSE; }

    float unknown_2[3 * vertices_count];
    if (!ReadFile(file, unknown_2, sizeof(unknown_2), NULL, NULL)) { return FALSE; }

    //
    // Unpack values
    //
    mesh->submesh_count = 1;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), 0, sizeof(MeshSubmesh)); // allocate single submesh
    if (mesh->submeshes == NULL) {
        return FALSE;
    }

    MeshSubmesh *submesh = &mesh->submeshes[0];
    submesh->point_count = vertices_count;
    submesh->points = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshPoint) * vertices_count);
    submesh->triangle_count = indices_count;
    submesh->triangles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshTriangle) * indices_count);
    for (int i = 0; i < vertices_count; ++i) {
        submesh->points[i].x = vertices[i].x;
        submesh->points[i].y = vertices[i].y;
        submesh->points[i].z = vertices[i].z;
    }
    for (int i = 0; i < indices_count; ++i) {
        submesh->triangles[i].a = indices[i].a;
        submesh->triangles[i].b = indices[i].b;
        submesh->triangles[i].c = indices[i].c;
    }
    submesh->texture = fallback_texture; // use fallback texture

    HeapFree(GetProcessHeap(), 0, vertices);
    HeapFree(GetProcessHeap(), 0, indices);

    return TRUE;
}

/**
 * Method 2: vertices data first, single material, with UV, with texture
 *
 * Examples:
 * - leaves01.mesh
 *
 * struct {
 *     int vertices_count;
       struct {
 *         float x, y, z;
 *         float u, v;
 *     } vertices[vertices_count];
 *     int indices_count;
 *     struct {
 *         short a, b, c;
 *     } indices[indices_count];
 *     char texture_name_length;
 *     char texture_name[texture_name_length];
 *     float unknown_2[9];
 * }
 */
BOOL Mesh_ReadMethod2(OUT Mesh *mesh, HANDLE file, DWORD file_size, const char *file_path)
{
    //
    // Read file
    //
    struct { float x, y, z, u, v; } *vertices;
    struct { short a, b, c; } *indices;

    // Read vertices
    int vertices_count = 0;
    if (!ReadFile(file, &vertices_count, sizeof(int), NULL, NULL)) { return FALSE; }

    DWORD vertices_size = vertices_count * sizeof(vertices[0]);
    if (vertices_size > file_size) { return FALSE; } // overflow check

    vertices = HeapAlloc(GetProcessHeap(), 0, vertices_size);
    for (int i = 0; i < vertices_count; ++i) {
        if (!ReadFile(file, &vertices[i].x, sizeof(float), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].y, sizeof(float), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].z, sizeof(float), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].u, sizeof(float), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].v, sizeof(float), NULL, NULL)) { return FALSE; }
    }

    // Read indices
    int indices_count = 0;
    if (!ReadFile(file, &indices_count, sizeof(int), NULL, NULL)) { return FALSE; }

    DWORD indices_size = indices_count * sizeof(indices[0]);
    if (indices_size > file_size) { return FALSE; } // overflow check

    indices = HeapAlloc(GetProcessHeap(), 0, indices_size);
    for (int i = 0; i < indices_count; ++i) {
        if (!ReadFile(file, &indices[i].a, sizeof(short), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &indices[i].b, sizeof(short), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &indices[i].c, sizeof(short), NULL, NULL)) { return FALSE; }
    }

    // Validate indices bounds
    for (int i = 0; i < indices_count; ++i) {
        if (indices[i].a < 0 || indices[i].a >= vertices_count) { return FALSE; }
        if (indices[i].b < 0 || indices[i].b >= vertices_count) { return FALSE; }
        if (indices[i].c < 0 || indices[i].c >= vertices_count) { return FALSE; }
    }

    // Read texture name
    BYTE texture_name_length;
    BYTE texture_name[0xFF];
    if (!ReadFile(file, &texture_name_length, sizeof(BYTE), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, texture_name, texture_name_length, NULL, NULL)) { return FALSE; }
    texture_name[texture_name_length] = '\0'; // null-terminate string

    float unknown_1[9];
    if (!ReadFile(file, unknown_1, sizeof(unknown_1), NULL, NULL)) { return FALSE; }

    //
    // Unpack values
    //
    mesh->submesh_count = 1;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), 0, sizeof(MeshSubmesh)); // allocate single submesh
    if (mesh->submeshes == NULL) {
        return FALSE;
    }

    MeshSubmesh *submesh = &mesh->submeshes[0];
    submesh->point_count = vertices_count;
    submesh->points = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshPoint) * vertices_count);
    submesh->triangle_count = indices_count;
    submesh->triangles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshTriangle) * indices_count);
    for (int i = 0; i < vertices_count; ++i) {
        submesh->points[i].x = vertices[i].x;
        submesh->points[i].y = vertices[i].y;
        submesh->points[i].z = vertices[i].z;
        submesh->points[i].u = vertices[i].u;
        submesh->points[i].v = vertices[i].v;
    }
    for (int i = 0; i < indices_count; ++i) {
        submesh->triangles[i].a = indices[i].a;
        submesh->triangles[i].b = indices[i].b;
        submesh->triangles[i].c = indices[i].c;
    }

    // Find texture path and load texture
    FindTexturePath(submesh->texture_path, texture_name, file_path);
    if (!Texture_Load(&submesh->texture, submesh->texture_path)) {
        submesh->texture = fallback_texture; // use fallback texture
    }

    HeapFree(GetProcessHeap(), 0, vertices);
    HeapFree(GetProcessHeap(), 0, indices);

    return TRUE;
}

/**
 * Method 5: header first, with UV, with texture
 *
 * Examples:
 * - pulya6.mesh
 *
 * struct {
 *     int unknown_1;
 *     int unknown_2;
 *     int unknown_3;
 *     byte unknown_4;
 *     byte texture_name_length;
 *     char texture_name[texture_name_length];
 *     byte unknown_5;
 *     int vertices_count;
 *     int indices_count;
 *     float unknown_6[19];
 *     byte unknown_7;
 *     float unknown_8;
 *     float unknown_9;
 *     float unknown_10;
 *     struct {
 *         short x;
 *         short y;
 *         short z;
 *         char unknown_1[4];
 *         float u;
 *         float v;
 *     } vertices[vertices_count];
 *     struct {
 *         short a;
 *         short b;
 *         short c;
 *     } indices[indices_count];
 *     float unknown_11;
 *     float unknown_12;
 *     float unknown_13;
 *     float unknown_14[12];
 * }
 */
BOOL Mesh_ReadMethod5(OUT Mesh *mesh, HANDLE file, DWORD file_size, const char *file_path)
{
    //
    // Structure
    //
    int unknown_1;
    int unknown_2;
    int unknown_3;
    BYTE unknown_4;
    BYTE texture_name_length;
    char texture_name[0xFF];
    BYTE unknown_5;
    int vertices_count;
    int indices_count;
    float unknown_6[19];
    BYTE unknown_7;
    float unknown_8;
    float unknown_9;
    float unknown_10;
    struct {
        short x, y, z;
        char unknown_1[4];
        float u, v;
    } *vertices;
    struct {
        short a, b, c;
    } *indices;
    float unknown_11;
    float unknown_12;
    float unknown_13;
    float unknown_14[12];

    //
    // Read file
    //
    // Header data
    if (!ReadFile(file, &unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_2, sizeof(int), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_3, sizeof(int), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_4, sizeof(BYTE), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &texture_name_length, sizeof(BYTE), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, texture_name, texture_name_length, NULL, NULL)) { return FALSE; }
    texture_name[texture_name_length] = '\0';
    if (!ReadFile(file, &unknown_5, sizeof(BYTE), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &vertices_count, sizeof(int), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &indices_count, sizeof(int), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, unknown_6, sizeof(unknown_6), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_7, sizeof(BYTE), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_8, sizeof(float), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_9, sizeof(float), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_10, sizeof(float), NULL, NULL)) { return FALSE; }

    // Allocate memory
    DWORD vertices_size = vertices_count * sizeof(vertices[0]);
    if (vertices_size > file_size) { return FALSE; } // overflow check
    vertices = HeapAlloc(GetProcessHeap(), 0, vertices_size);
    if (vertices == NULL) { return FALSE; }

    DWORD indices_size = indices_count * sizeof(indices[0]);
    if (indices_size > file_size) { return FALSE; } // overflow check
    indices = HeapAlloc(GetProcessHeap(), 0, indices_size);
    if (indices == NULL) { return FALSE; }

    // Read geometry data
    for (int i = 0; i < vertices_count; ++i) {
        if (!ReadFile(file, &vertices[i].x, sizeof(short), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].y, sizeof(short), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].z, sizeof(short), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].unknown_1, sizeof(vertices[0].unknown_1), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].u, sizeof(float), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &vertices[i].v, sizeof(float), NULL, NULL)) { return FALSE; }
    }
    for (int i = 0; i < indices_count; ++i) {
        if (!ReadFile(file, &indices[i].a, sizeof(short), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &indices[i].b, sizeof(short), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &indices[i].c, sizeof(short), NULL, NULL)) { return FALSE; }
    }

    // Validate indices bounds
    for (int i = 0; i < indices_count; ++i) {
        if (indices[i].a < 0 || indices[i].a >= vertices_count) { return FALSE; }
        if (indices[i].b < 0 || indices[i].b >= vertices_count) { return FALSE; }
        if (indices[i].c < 0 || indices[i].c >= vertices_count) { return FALSE; }
    }

    if (!ReadFile(file, &unknown_11, sizeof(float), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_12, sizeof(float), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_13, sizeof(float), NULL, NULL)) { return FALSE; }
    if (!ReadFile(file, &unknown_14, sizeof(unknown_14), NULL, NULL)) { return FALSE; }

    //
    // Unpack values
    //
    mesh->submesh_count = 1;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), 0, sizeof(MeshSubmesh)); // allocate single submesh
    if (mesh->submeshes == NULL) {
        return FALSE;
    }

    MeshSubmesh *submesh = &mesh->submeshes[0];
    submesh->point_count = vertices_count;
    submesh->points = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshPoint) * vertices_count);
    submesh->triangle_count = indices_count;
    submesh->triangles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshTriangle) * indices_count);
    for (int i = 0; i < vertices_count; ++i) {
        submesh->points[i].x = (float)vertices[i].x / INT16_MAX; // unpack coordinates
        submesh->points[i].y = (float)vertices[i].y / INT16_MAX;
        submesh->points[i].z = (float)vertices[i].z / INT16_MAX;
        submesh->points[i].u = vertices[i].u;
        submesh->points[i].v = vertices[i].v;
    }
    for (int i = 0; i < indices_count; ++i) {
        submesh->triangles[i].a = indices[i].a;
        submesh->triangles[i].b = indices[i].b;
        submesh->triangles[i].c = indices[i].c;
    }

    // Find texture path and load texture
    FindTexturePath(submesh->texture_path, texture_name, file_path);
    if (!Texture_Load(&submesh->texture, submesh->texture_path)) {
        submesh->texture = fallback_texture; // use fallback texture
    }

    HeapFree(GetProcessHeap(), 0, vertices);
    HeapFree(GetProcessHeap(), 0, indices);

    return TRUE;
}

BOOL Mesh_Read(OUT Mesh *mesh, HANDLE file, const char *file_path)
{
    DWORD file_size = GetFileSize(file, NULL);

    // Depending on first int value determine the mesh type
    int first_value;
    if (!ReadFile(file, &first_value, sizeof(int), NULL, NULL)) { return FALSE; }

    SetFilePointer(file, 0, NULL, FILE_BEGIN); // start file reading from beginning
    
    if (HIBYTE(LOWORD(first_value)) == 'z') { // "pz  " - ZIP compression
        return FALSE; // TODO: compressed models not supported yet
    }
    if (first_value == 0) {
        return Mesh_ReadMethod6(mesh, file, file_size, file_path);
    } else {
        BOOL success = Mesh_ReadMethod4(mesh, file, file_size, file_path); // try method 4
        if (!success) {
            // If previous method failed, reset file pointer and try method 1
            SetFilePointer(file, 0, NULL, FILE_BEGIN);
            success = Mesh_ReadMethod1(mesh, file, file_size, file_path);
        }
        if (!success) {
            // If previous method failed, reset file pointer and try method 2
            SetFilePointer(file, 0, NULL, FILE_BEGIN);
            success = Mesh_ReadMethod2(mesh, file, file_size, file_path);
        }
        if (!success) {
            // If previous method failed, reset file pointer and try method 5
            SetFilePointer(file, 0, NULL, FILE_BEGIN);
            success = Mesh_ReadMethod5(mesh, file, file_size, file_path);
        }
        return success;
    }
}

BOOL Mesh_LoadFromPathologicFormat(Mesh *mesh, const char *path)
{
    // Open file for reading
    HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    // Read mesh
    BOOL success = Mesh_Read(mesh, file, path);
    if (!success) {
        ZeroMemory(mesh, sizeof(Mesh));
    }

    CloseHandle(file);

    return success;
}
