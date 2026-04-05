#include "mesh.h"

#include <shlwapi.h>
#include <stdint.h>
#include <math.h>

#include <zlib.h>

#include "draw.h"

typedef struct { BYTE *data; DWORD size; DWORD offset; } FileBuffer;

DWORD FileBuffer_GetRemainingSize(FileBuffer *buffer)
{
    return buffer->size - buffer->offset;
}

BOOL FileBuffer_SetOffset(FileBuffer *buffer, DWORD offset)
{
    if (offset > buffer->size) {
        return FALSE;
    }
    buffer->offset = offset;
    return TRUE;
}

BOOL FileBuffer_Skip(FileBuffer *buffer, DWORD offset)
{
    if ((buffer->offset + offset) > buffer->size) { // TODO: may overflow
        return FALSE;
    }
    buffer->offset += offset;
    return TRUE;
}

BOOL FileBuffer_ReadChar(FileBuffer *buffer, OUT char *out)
{
    *out = *(char *)(buffer->data + buffer->offset);
    DWORD new_offset = buffer->offset + sizeof(char);
    if (new_offset > buffer->size) { return FALSE; }
    buffer->offset = new_offset;
    return TRUE;
}

BOOL FileBuffer_ReadByte(FileBuffer *buffer, OUT BYTE *out)
{
    *out = *(buffer->data + buffer->offset);
    DWORD new_offset = buffer->offset + sizeof(BYTE);
    if (new_offset > buffer->size) { return FALSE; }
    buffer->offset = new_offset;
    return TRUE;
}

BOOL FileBuffer_ReadShort(FileBuffer *buffer, OUT short *out)
{
    *out = *(short *)(buffer->data + buffer->offset);
    DWORD new_offset = buffer->offset + sizeof(short);
    if (new_offset > buffer->size) { return FALSE; }
    buffer->offset = new_offset;
    return TRUE;
}

BOOL FileBuffer_ReadWord(FileBuffer *buffer, OUT WORD *out)
{
    *out = *(WORD *)(buffer->data + buffer->offset);
    DWORD new_offset = buffer->offset + sizeof(WORD);
    if (new_offset > buffer->size) { return FALSE; }
    buffer->offset = new_offset;
    return TRUE;
}

BOOL FileBuffer_ReadInt(FileBuffer *buffer, OUT int *out)
{
    *out = *(int *)(buffer->data + buffer->offset);
    DWORD new_offset = buffer->offset + sizeof(int);
    if (new_offset > buffer->size) { return FALSE; }
    buffer->offset = new_offset;
    return TRUE;
}

BOOL FileBuffer_ReadFloat(FileBuffer *buffer, OUT float *out)
{
    *out = *(float *)(buffer->data + buffer->offset);
    DWORD new_offset = buffer->offset + sizeof(float);
    if (new_offset > buffer->size) { return FALSE; }
    buffer->offset = new_offset;
    return TRUE;
}

BOOL FileBuffer_ReadFloats(FileBuffer *buffer, OUT float *str, int str_length)
{
    for (int i = 0; i < str_length; ++i) {
        if (!FileBuffer_ReadFloat(buffer, &str[i])) { return FALSE; }
    }
    return TRUE;
}

BOOL FileBuffer_ReadChars(FileBuffer* buffer, OUT char *str, int str_length)
{
    for (int i = 0; i < str_length; ++i) {
        if (!FileBuffer_ReadChar(buffer, &str[i])) { return FALSE; }
    }
    return TRUE;
}

BOOL FileBuffer_ReadBytes(FileBuffer* buffer, OUT BYTE *str, int str_length)
{
    for (int i = 0; i < str_length; ++i) {
        if (!FileBuffer_ReadByte(buffer, &str[i])) { return FALSE; }
    }
    return TRUE;
}

//
// Notes:
// - In .mesh file geometry data grouped by unique material (usually it's a diffuse texture)
//   and for simplicity I called every block of grouped geometry data a "submesh"
//
// material_type=33 - isobor_lampa.mesh
// material_type=33 - icot_eva_Kerosinka.mesh
// material_type=17 - inv_grass_white_plet.mesh
//

BOOL Mesh_ReadSubmeshHeader(OUT SubmeshHeader *submesh_header, FileBuffer *buffer, BOOL with_subblocks)
{
    // Submesh ID
    if (!FileBuffer_ReadInt(buffer, &submesh_header->id)) { return FALSE; }

    // Material type
    if (!FileBuffer_ReadInt(buffer, &submesh_header->material_type)) { return FALSE; }
    if (submesh_header->material_type == 0) {
        // ??? unknown byte
        if (!FileBuffer_ReadChar(buffer, &submesh_header->unknown_3)) { return FALSE; }
        // ??? unknown 6 floats
        if (!FileBuffer_ReadFloats(buffer, submesh_header->unknown_4, 6)) { return FALSE; }
    } else {
        if (submesh_header->material_type == 1 || submesh_header->material_type == 17) {
            // ??? unknown byte
            if (!FileBuffer_ReadChar(buffer, &submesh_header->unknown_3)) { return FALSE; }
        }

        // Read texture name
        if (!FileBuffer_ReadByte(buffer, &submesh_header->texture_name_length)) { return FALSE; }
        if (!FileBuffer_ReadChars(buffer, submesh_header->texture_name, submesh_header->texture_name_length)) { return FALSE; }
        submesh_header->texture_name[submesh_header->texture_name_length] = '\0';

        // ???
        if (submesh_header->material_type == 3) { // tower_stone.mesh
            BYTE unknown_3;
            if (!FileBuffer_ReadByte(buffer, &unknown_3)) { return FALSE; }
            BYTE bump_name_length;
            if (!FileBuffer_ReadByte(buffer, &bump_name_length)) { return FALSE; }
            char bump_name[bump_name_length];
            if (!FileBuffer_ReadChars(buffer, bump_name, bump_name_length)) { return FALSE; }
        }
        if (submesh_header->material_type == 1 || submesh_header->material_type == 33) {
            char unknown_4;
            if (!FileBuffer_ReadChar(buffer, &unknown_4)) { return FALSE; }
        }
        if (submesh_header->material_type == 5) {
            char unknown_4;
            if (!FileBuffer_ReadChar(buffer, &unknown_4)) { return FALSE; }
            float unknown_4floats[4];
            if (!FileBuffer_ReadFloats(buffer, unknown_4floats, ARRAYSIZE(unknown_4floats))) { return FALSE; }
        }
    }

    // Read vertices and indices count
    if (!FileBuffer_ReadInt(buffer, &submesh_header->point_count)) { return FALSE; }
    if (!FileBuffer_ReadInt(buffer, &submesh_header->indices_count)) { return FALSE; }

    // Sanity checks
    if ((submesh_header->point_count * sizeof(int)) > FileBuffer_GetRemainingSize(buffer)) { return FALSE; }
    if ((submesh_header->indices_count * sizeof(int)) > FileBuffer_GetRemainingSize(buffer)) { return FALSE; }

    // Occlusion data (sphere with oriented bounding box)
    if (!FileBuffer_ReadFloats(buffer, (float *)&submesh_header->sphere_with_obb, 19)) { return FALSE; }

    // ??? unknown blocks
    if (with_subblocks) {
        if (submesh_header->material_type == 0 || submesh_header->material_type == 3 || submesh_header->material_type == 5 || submesh_header->material_type == 1 || submesh_header->material_type == 17) {
            int unknown_blocks_count;
            if (!FileBuffer_ReadInt(buffer, &unknown_blocks_count)) { return FALSE; }
            // Sanity check
            if ((unknown_blocks_count * sizeof(SubmeshHeaderUnknownBlock)) > FileBuffer_GetRemainingSize(buffer)) { return FALSE; }

            submesh_header->unknown_blocks_count = unknown_blocks_count;
            if (unknown_blocks_count > 0) {
                submesh_header->unknown_blocks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeaderUnknownBlock) * unknown_blocks_count);
                for (int j = 0; j < unknown_blocks_count; ++j) {
                    SubmeshHeaderUnknownBlock *unknown_block = &submesh_header->unknown_blocks[j];

                    if (!FileBuffer_ReadInt(buffer, &unknown_block->unknown_2)) { return FALSE; }
                    if (!FileBuffer_ReadFloats(buffer, unknown_block->unknown_15floats, 15)) { return FALSE; }
                    if (!FileBuffer_ReadInt(buffer, &unknown_block->subblocks_count)) { return FALSE; }
                    if (unknown_block->subblocks_count != 0) {
                        unknown_block->subblocks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeaderUnknownBlockSubblock) * unknown_block->subblocks_count);

                        for (int k = 0; k < unknown_block->subblocks_count; ++k) {
                            SubmeshHeaderUnknownBlockSubblock *subblock = &unknown_block->subblocks[k];

                            if (!FileBuffer_ReadInt(buffer, &subblock->unknown_4)) { return FALSE; }
                            if (!FileBuffer_ReadInt(buffer, &subblock->unknown_5)) { return FALSE; }
                            if (!FileBuffer_ReadInt(buffer, &subblock->unknown_array_length_1)) { return FALSE; }
                            FileBuffer_Skip(buffer, sizeof(char) * subblock->unknown_array_length_1); // skip
                            if (!FileBuffer_ReadInt(buffer, &subblock->unknown_array_length_2)) { return FALSE; }
                            FileBuffer_Skip(buffer, sizeof(char) * subblock->unknown_array_length_2); // skip
                            if (!FileBuffer_ReadInt(buffer, &subblock->unknown_array_length_3)) { return FALSE; }
                            if (!FileBuffer_ReadInt(buffer, &subblock->unknown_array_length_4)) { return FALSE; }
                            if (submesh_header->material_type != 3) { // tower_stone.mesh
                                FileBuffer_Skip(buffer, sizeof(short) * 4 * subblock->unknown_array_length_4); // skip
                            }
                            if (!FileBuffer_ReadFloat(buffer, &subblock->unknown_13)) { return FALSE; }
                        }
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

BOOL Mesh_ReadStructureType3(OUT Mesh *mesh, FileBuffer *buffer, const char *file_path, BOOL with_subblocks)
{
    if (with_subblocks) {
        int unknown_1 = 0;
        if (!FileBuffer_ReadInt(buffer, &unknown_1)) { return FALSE; }
    }

    int submesh_count = 0;
    if (!FileBuffer_ReadInt(buffer, &submesh_count)) { return FALSE; }

    // Allocate memory to store submeshes
    mesh->submesh_count = submesh_count;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeader) * submesh_count);

    // Read submesh headers
    for (int i = 0; i < mesh->submesh_count; ++i) {
        SubmeshHeader *submesh_header = &mesh->submeshes[i];
        if (!Mesh_ReadSubmeshHeader(submesh_header, buffer, with_subblocks)) { // read every submesh header block
            return FALSE;
        }
    }

    // Load all textures specified in submesh headers
    for (int i = 0; i < mesh->submesh_count; ++i) {
        SubmeshHeader *submesh = &mesh->submeshes[i];

        if (submesh->material_type != 0) {
            // Find texture path
            if (!FindTexturePath(submesh->texture_path, submesh->texture_name, file_path)) {
                return FALSE;
            }
            // Load texture
            if (!Texture_Load(&submesh->texture, submesh->texture_path)) {
                submesh->texture = fallback_texture; // use fallback texture
            }
        }
    }

    // Read blocks data
    for (int i = 0; i < mesh->submesh_count; ++i) {
        SubmeshHeader *submesh = &mesh->submeshes[i];

        if (!FileBuffer_ReadByte(buffer, &submesh->vertices_type)) { return FALSE; }

        if (submesh->vertices_type != 0) {
            // World-space coordinates
            if (!FileBuffer_ReadFloat(buffer, &submesh->world_x)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &submesh->world_y)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &submesh->world_z)) { return FALSE; }
        }

        // Read points
        submesh->points = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshPoint) * submesh->point_count);
        for (int j = 0; j < submesh->point_count; ++j) {
            MeshPoint *point = &submesh->points[j];

            if (submesh->vertices_type == MESH_VERTICES_TYPE_FLOAT) {
                // FLOAT
                if (!FileBuffer_ReadFloat(buffer, &point->pos.x)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &point->pos.y)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &point->pos.z)) { return FALSE; }
            } else {
                // SHORT
                if (!FileBuffer_ReadShort(buffer, &point->pos_short.x)) { return FALSE; }
                if (!FileBuffer_ReadShort(buffer, &point->pos_short.y)) { return FALSE; }
                if (!FileBuffer_ReadShort(buffer, &point->pos_short.z)) { return FALSE; }

                point->pos.x = (float)point->pos_short.x / (float)INT16_MAX; // unpack 16-bit short coordinates
                point->pos.y = (float)point->pos_short.y / (float)INT16_MAX;
                point->pos.z = (float)point->pos_short.z / (float)INT16_MAX;
            }

            if (submesh->material_type == 0) {
                if (!FileBuffer_ReadShort(buffer, &point->norm_short.x)) { return FALSE; }
                if (!FileBuffer_ReadShort(buffer, &point->norm_short.y)) { return FALSE; }
                float nx = (float)point->norm_short.x / (float)INT16_MAX; // unpack 16-bit short normal
                float ny = (float)point->norm_short.y / (float)INT16_MAX;
                float nz = sqrtf(1.0f - nx*nx - ny*ny);
                point->norm.x = nx;
                point->norm.y = ny;
                point->norm.z = nz;
            }
            if (submesh->material_type == 1) {

                if (!FileBuffer_ReadShort(buffer, &point->norm_short.x)) { return FALSE; }
                if (!FileBuffer_ReadShort(buffer, &point->norm_short.y)) { return FALSE; }
                float nx = (float)point->norm_short.x / (float)INT16_MAX; // unpack 16-bit short normal
                float ny = (float)point->norm_short.y / (float)INT16_MAX;
                float nz = sqrtf(1.0f - nx*nx - ny*ny);
                point->norm.x = nx;
                point->norm.y = ny;
                point->norm.z = nz;

                if (!FileBuffer_ReadFloat(buffer, &point->u)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &point->v)) { return FALSE; }
            }
            if (submesh->material_type == 3) { // tower_stone.mesh
                float unknown_1;
                float unknown_2;
                float unknown_3;
                float unknown_4;
                float unknown_5;
                float unknown_6;
                float unknown_7;
                float unknown_8;
                float unknown_9;
                float unknown_10;
                if (!FileBuffer_ReadFloat(buffer, &unknown_1)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &unknown_2)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &unknown_3)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &unknown_4)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &unknown_5)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &unknown_6)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &unknown_7)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &unknown_8)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &unknown_9)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &unknown_10)) { return FALSE; }
            }
            if (submesh->material_type == 5) {

                if (!FileBuffer_ReadShort(buffer, &point->norm_short.x)) { return FALSE; }
                if (!FileBuffer_ReadShort(buffer, &point->norm_short.y)) { return FALSE; }
                float nx = (float)point->norm_short.x / (float)INT16_MAX; // unpack 16-bit short normal
                float ny = (float)point->norm_short.y / (float)INT16_MAX;
                float nz = sqrtf(1.0f - nx*nx - ny*ny);
                point->norm.x = nx;
                point->norm.y = ny;
                point->norm.z = nz;

                if (!FileBuffer_ReadFloat(buffer, &point->u)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &point->v)) { return FALSE; }
            }
            if (submesh->material_type == 17) {

                if (!FileBuffer_ReadShort(buffer, &point->norm_short.x)) { return FALSE; }
                if (!FileBuffer_ReadShort(buffer, &point->norm_short.y)) { return FALSE; }
                float nx = (float)point->norm_short.x / (float)INT16_MAX; // unpack 16-bit short normal
                float ny = (float)point->norm_short.y / (float)INT16_MAX;
                float nz = sqrtf(1.0f - nx*nx - ny*ny);
                point->norm.x = nx;
                point->norm.y = ny;
                point->norm.z = nz;

                if (!FileBuffer_ReadFloat(buffer, &point->u)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &point->v)) { return FALSE; }
            }
            if (submesh->material_type == 33) {
                if (!FileBuffer_ReadFloat(buffer, &point->u)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &point->v)) { return FALSE; }
            }
            if (submesh->material_type == 64) {
                int unknown1;
                int unknown2;
                int unknown3;
                if (!FileBuffer_ReadInt(buffer, &unknown1)) { return FALSE; }
                if (!FileBuffer_ReadInt(buffer, &unknown2)) { return FALSE; }
                if (!FileBuffer_ReadInt(buffer, &unknown3)) { return FALSE; }
            }
            if (submesh->unknown_3 == 1) {
                if (!FileBuffer_ReadShort(buffer, &point->norm_short.x)) { return FALSE; }
                if (!FileBuffer_ReadShort(buffer, &point->norm_short.y)) { return FALSE; }
                float nx = (float)point->norm_short.x / (float)INT16_MAX; // unpack 16-bit short normal
                float ny = (float)point->norm_short.y / (float)INT16_MAX;
                float nz = sqrtf(1.0f - nx*nx - ny*ny);
                point->norm.x = nx;
                point->norm.y = ny;
                point->norm.z = nz;
            }
        }

        if (submesh->material_type == 3) { // tower_stone.mesh
            // char unknown_2[5858];
            FileBuffer_Skip(buffer, 5858);
        }

        // Read indices
        submesh->indices = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshTriangle) * submesh->indices_count);
        for (int j = 0; j < submesh->indices_count; ++j) {
            MeshTriangle *triangle = &submesh->indices[j];

            // Read indices
            WORD a, b, c;
            if (!FileBuffer_ReadWord(buffer, &a)) { return FALSE; }
            if (!FileBuffer_ReadWord(buffer, &b)) { return FALSE; }
            if (!FileBuffer_ReadWord(buffer, &c)) { return FALSE; }

            // Validate indices bounds
            if (a < 0 || a >= submesh->point_count) { return FALSE; }
            if (b < 0 || b >= submesh->point_count) { return FALSE; }
            if (c < 0 || c >= submesh->point_count) { return FALSE; }

            triangle->a = a;
            triangle->b = b;
            triangle->c = c;
        }

        // ???
        if (with_subblocks) {
            for (int i2 = 0; i2 < submesh->unknown_blocks_count; ++i2) {
                SubmeshHeaderUnknownBlock *unknown_block = &submesh->unknown_blocks[i2];

                for (int i3 = 0; i3 < unknown_block->subblocks_count; ++i3) {
                    SubmeshHeaderUnknownBlockSubblock *subblock = &unknown_block->subblocks[i3];

                    for (int i4 = 0; i4 < subblock->unknown_array_length_3; ++i4) {

                        if (submesh->material_type == 3) { // tower_stone.mesh

                            float unknown_floats[12];
                            if (!FileBuffer_ReadFloats(buffer, unknown_floats, ARRAYSIZE(unknown_floats))) { return FALSE; }
                            WORD unknown_short;
                            if (!FileBuffer_ReadWord(buffer, &unknown_short)) { return FALSE; }

                        } else {
                            WORD unknown_short;
                            if (!FileBuffer_ReadWord(buffer, &unknown_short)) { return FALSE; }

                            float unknown_float4;
                            if (!FileBuffer_ReadFloat(buffer, &unknown_float4)) { return FALSE; }
                            float unknown_float5;
                            if (!FileBuffer_ReadFloat(buffer, &unknown_float5)) { return FALSE; }

                            if (submesh->unknown_3 == 1 || submesh->unknown_3 == 17) {
                                short unknown_short1;
                                if (!FileBuffer_ReadShort(buffer, &unknown_short1)) { return FALSE; }
                                short unknown_short2;
                                if (!FileBuffer_ReadShort(buffer, &unknown_short2)) { return FALSE; }
                            }
                        }
                    }

                    if (submesh->material_type == 3) { // tower_stone.mesh
                        // char unknown_5[2818];
                        FileBuffer_Skip(buffer, 2818);
                        // short unknown_6[submeshes[i].unknown_block[j].subblocks[k].unknown_array_length_3];
                        FileBuffer_Skip(buffer, sizeof(WORD) * subblock->unknown_array_length_3);
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
    int material_flags;
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
        char unknown_1[4]; // only for (material_flags == 1)
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
BOOL Mesh_ReadMethod4(OUT Mesh *mesh, FileBuffer *buffer, const char *file_path)
{
    //
    // Read "Block 1" data (purpose of this block is unclean, perhaps objects, particles, or lighting data)
    //

    // Read block count
    int block1_count;
    if (!FileBuffer_ReadInt(buffer, &block1_count)) { return FALSE; }
    if ((block1_count * sizeof(Block1)) > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check

    // Read blocks data
    Block1 block1;
    for (int i1 = 0; i1 < block1_count; ++i1) {

        // Read "block1.unknown_1" block until "block1.unknown_1.unknown_1" is not 0
        while (1) {
            if (!FileBuffer_ReadByte(buffer, &block1.unknown_1.unknown_1)) { return FALSE; }
            if (block1.unknown_1.unknown_1 == 0) {
                break; // done reading
            }

            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_1.unknown_2)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_1.unknown_3)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_1.unknown_4)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_1.unknown_5)) { return FALSE; }
        }

        // Read current block index
        if (!FileBuffer_ReadInt(buffer, &block1.index)) { return FALSE; } // read current block index
        // Safety check: block index specified in the file should match the currently reading block index
        if (block1.index != i1) {
            return FALSE; // indices do not match
        }

        // Read "block1.unknown_2" block
        if (!FileBuffer_ReadInt(buffer, &block1.unknown_2.unknown_1_count)) { return FALSE; }
        for (int i2 = 0; i2 < block1.unknown_2.unknown_1_count; ++i2) {
            if (!FileBuffer_ReadInt(buffer, &block1.unknown_2.unknown_1.unknown_1)) { return FALSE; }
            if (!FileBuffer_ReadInt(buffer, &block1.unknown_2.unknown_1.unknown_2)) { return FALSE; }
        }
        if (!FileBuffer_ReadFloat(buffer, &block1.unknown_2.unknown_2)) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &block1.unknown_2.unknown_3)) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &block1.unknown_2.unknown_4)) { return FALSE; }

        if (!FileBuffer_ReadInt(buffer, &block1.unknown_3_length)) { return FALSE; }
        FileBuffer_Skip(buffer, block1.unknown_3_length * sizeof(block1.unknown_3[0])); // skip unknown data
        if (!FileBuffer_ReadInt(buffer, &block1.unknown_4_length)) { return FALSE; }
        FileBuffer_Skip(buffer, block1.unknown_4_length * sizeof(block1.unknown_4[0])); // skip unknown data

        // Read "block1.unknown_5" block
        if (!FileBuffer_ReadInt(buffer, &block1.unknown_5_count)) { return FALSE; }
        if (block1.unknown_5_count * sizeof(block1.unknown_5_1) > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check
        for (int i2 = 0; i2 < block1.unknown_5_count; ++i2) {
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_5_1.unknown_1)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_5_1.unknown_2)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_5_1.unknown_3)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_5_1.unknown_4)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_5_1.unknown_5)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_5_1.unknown_6)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_5_1.unknown_7)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_5_1.unknown_8)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block1.unknown_5_1.unknown_9)) { return FALSE; }
        }
        for (int i2 = 0; i2 < block1.unknown_5_count; ++i2) {
            if (!FileBuffer_ReadByte(buffer, &block1.unknown_5_2)) { return FALSE; }
        }
    }

    //
    // Read "Block 2" data
    //
    int block2_count;
    if (!FileBuffer_ReadInt(buffer, &block2_count)) { return FALSE; }

    int unknown_1; // unknown values that somehow corresponding with "Block 2" data
    for (int i = 0; i < block2_count; ++i) {
        if (!FileBuffer_ReadInt(buffer, &unknown_1)) { return FALSE; }
    }

    //
    // Read "Block 2" data
    //
    int block3_count;
    if (!FileBuffer_ReadInt(buffer, &block3_count)) { return FALSE; }
    if (block3_count * sizeof(Block3) > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check

    Block3 *blocks3 = HeapAlloc(GetProcessHeap(), 0, block3_count * sizeof(Block3)); // TODO: read directly to submesh header
    for (int i1 = 0; i1 < block3_count; ++i1) {
        Block3 *block3 = &blocks3[i1];

        if (!FileBuffer_ReadInt(buffer, &block3->triangles_count_1)) { return FALSE; }
        if (!FileBuffer_ReadInt(buffer, &block3->material_flags)) { return FALSE; }

        if (!FileBuffer_ReadByte(buffer, &block3->texture_name_length)) { return FALSE; }
        if (!FileBuffer_ReadChars(buffer, block3->texture_name, block3->texture_name_length)) { return FALSE; }
        block3->texture_name[block3->texture_name_length] = 0;

        if (!FileBuffer_ReadInt(buffer, &block3->triangles_count_2)) { return FALSE; }
        if (!FileBuffer_ReadByte(buffer, &block3->vertices_type)) { return FALSE; }
        if (block3->vertices_type == 1) {
            if (!FileBuffer_ReadFloat(buffer, &block3->unknown_4)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block3->unknown_5)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block3->unknown_6)) { return FALSE; }
        }

        int vertices_count = block3->triangles_count_2 * 3;
        size_t vertex_struct_size = block3->vertices_type == 1 ? 12 : 16; // size of vertex element
        if ((vertices_count * vertex_struct_size) > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check
        block3->vertices = HeapAlloc(GetProcessHeap(), 0, vertices_count * sizeof(block3->vertices[0]));
        if (block3->vertices == NULL) { return FALSE; }
        for (int i2 = 0; i2 < vertices_count; ++i2) {
            if (block3->vertices_type == 1) {
                if (!FileBuffer_ReadShort(buffer, &block3->vertices[i2].pos_short.x)) { return FALSE; }
                if (!FileBuffer_ReadShort(buffer, &block3->vertices[i2].pos_short.y)) { return FALSE; }
                if (!FileBuffer_ReadShort(buffer, &block3->vertices[i2].pos_short.z)) { return FALSE; }
            } else {
                if (!FileBuffer_ReadFloat(buffer, &block3->vertices[i2].pos_float.x)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &block3->vertices[i2].pos_float.y)) { return FALSE; }
                if (!FileBuffer_ReadFloat(buffer, &block3->vertices[i2].pos_float.z)) { return FALSE; }
            }
            if (block3->material_flags == 1) {
                if (!FileBuffer_ReadChars(buffer, block3->vertices[i2].unknown_1, sizeof(block3->vertices[i2].unknown_1))) { return FALSE; }
            }
            if (!FileBuffer_ReadFloat(buffer, &block3->vertices[i2].u)) { return FALSE; }
            if (!FileBuffer_ReadFloat(buffer, &block3->vertices[i2].v)) { return FALSE; }
        }

        if (block3->material_flags == 1) {
            for (int i2 = 0; i2 < block2_count; ++i2) {
                int block4_count;
                if (!FileBuffer_ReadInt(buffer, &block4_count)) { return FALSE; }
                if (block4_count <= 0) {
                    continue; // read the next data only if "block4_count" value greater than zero
                }

                if (block4_count * sizeof(Block4) > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check

                Block4 block4;
                for (int i3 = 0; i3 < block4_count; ++i3) {
                    if (!FileBuffer_ReadInt(buffer, &block4.unknown_1)) { return FALSE; }
                    if (!FileBuffer_ReadInt(buffer, &block4.unknown_2)) { return FALSE; }

                    if (!FileBuffer_ReadInt(buffer, &block4.unknown_3_length)) { return FALSE; }
                    FileBuffer_Skip(buffer, block4.unknown_3_length); // skip unknown data

                    if (!FileBuffer_ReadInt(buffer, &block4.unknown_4_length)) { return FALSE; }
                    FileBuffer_Skip(buffer, block4.unknown_4_length); // skip unknown data

                    if (!FileBuffer_ReadInt(buffer, &block4.unknown_5_count)) { return FALSE; } // note that this value is multiplied by 3 for the following loop
                    for (int i4 = 0; i4 < block4.unknown_5_count * 3; ++i4) {
                        if (!FileBuffer_ReadShort(buffer, &block4.unknown_5.unknown_1)) { return FALSE; }
                        if (!FileBuffer_ReadFloat(buffer, &block4.unknown_5.unknown_2)) { return FALSE; }
                        if (!FileBuffer_ReadFloat(buffer, &block4.unknown_5.unknown_3)) { return FALSE; }
                        if (!FileBuffer_ReadFloat(buffer, &block4.unknown_5.unknown_4)) { return FALSE; }
                        if (!FileBuffer_ReadFloat(buffer, &block4.unknown_5.unknown_5)) { return FALSE; }
                    }
                    if (!FileBuffer_ReadFloat(buffer, &block4.unknown_6)) { return FALSE; }
                }

                for (int i3 = 0; i3 < block3->triangles_count_2; ++i3) {
                    if (!FileBuffer_ReadInt(buffer, &block4.unknown_7.unknown_1)) { return FALSE; }
                    if (!FileBuffer_ReadInt(buffer, &block4.unknown_7.unknown_2)) { return FALSE; }
                }
            }
        }
    }

    //
    // Unpack data
    //
    mesh->submesh_count = block3_count;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeader) * mesh->submesh_count);
    if (mesh->submeshes == NULL) {
        return FALSE;
    }

    // Load textures first
    for (int i1 = 0; i1 < mesh->submesh_count; ++i1) {
        SubmeshHeader *submesh = &mesh->submeshes[i1];
        Block3 *block3 = &blocks3[i1];

        if (   !FindTexturePath(submesh->texture_path, block3->texture_name, file_path)
            || !Texture_Load(&submesh->texture, submesh->texture_path)) {
            submesh->texture = fallback_texture; // use fallback texture
        }
    }

    for (int i1 = 0; i1 < mesh->submesh_count; ++i1) {
        SubmeshHeader *submesh = &mesh->submeshes[i1];
        Block3 *block3 = &blocks3[i1];

        submesh->point_count = block3->triangles_count_2 * 3;
        submesh->points = HeapAlloc(GetProcessHeap(), 0, submesh->point_count * sizeof(MeshPoint));
        if (submesh->points == NULL) {
            return FALSE;
        }
        for (int i2 = 0; i2 < submesh->point_count; ++i2) {
            if (block3->vertices_type == 1) {
                // submesh->points[i2].pos.x = (float)block3->vertices[i2].pos_short.x / INT16_MAX; // unpack coordinates
                // submesh->points[i2].pos.y = (float)block3->vertices[i2].pos_short.y / INT16_MAX;
                // submesh->points[i2].pos.z = (float)block3->vertices[i2].pos_short.z / INT16_MAX;
                submesh->points[i2].pos.x = (float)block3->vertices[i2].pos_short.x; // unpack coordinates
                submesh->points[i2].pos.y = (float)block3->vertices[i2].pos_short.y;
                submesh->points[i2].pos.z = (float)block3->vertices[i2].pos_short.z;
            } else {
                submesh->points[i2].pos.x = block3->vertices[i2].pos_float.x;
                submesh->points[i2].pos.y = block3->vertices[i2].pos_float.y;
                submesh->points[i2].pos.z = block3->vertices[i2].pos_float.z;
            }
            submesh->points[i2].u = block3->vertices[i2].u;
            submesh->points[i2].v = block3->vertices[i2].v;
        }

        submesh->indices_count = block3->triangles_count_2;
        submesh->indices = HeapAlloc(GetProcessHeap(), 0, submesh->indices_count * sizeof(MeshTriangle));
        if (submesh->indices == NULL) {
            return FALSE;
        }
        for (int i2 = 0; i2 < submesh->indices_count; ++i2) {
            submesh->indices[i2].a = ((i2 * 3) + 0) % submesh->point_count; // calculate indices
            submesh->indices[i2].b = ((i2 * 3) + 1) % submesh->point_count;
            submesh->indices[i2].c = ((i2 * 3) + 2) % submesh->point_count;
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
BOOL Mesh_ReadMethod1(OUT Mesh *mesh, FileBuffer *buffer, const char *file_path)
{
    //
    // Read file
    //
    struct { float x, y, z; } *vertices;
    struct { short a, b, c; } *indices;

    // Read vertices
    int vertices_count = 0;
    if (!FileBuffer_ReadInt(buffer, &vertices_count)) { return FALSE; }

    DWORD vertices_size = vertices_count * sizeof(vertices[0]);
    if (vertices_size > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check

    vertices = HeapAlloc(GetProcessHeap(), 0, vertices_size);
    for (int i = 0; i < vertices_count; ++i) {
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].x)) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].y)) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].z)) { return FALSE; }
    }

    // Read indices
    int indices_count = 0;
    if (!FileBuffer_ReadInt(buffer, &indices_count)) { return FALSE; }

    DWORD indices_size = indices_count * sizeof(indices[0]);
    if (indices_size > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check

    indices = HeapAlloc(GetProcessHeap(), 0, indices_size);
    for (int i = 0; i < indices_count; ++i) {
        if (!FileBuffer_ReadShort(buffer, &indices[i].a)) { return FALSE; }
        if (!FileBuffer_ReadShort(buffer, &indices[i].b)) { return FALSE; }
        if (!FileBuffer_ReadShort(buffer, &indices[i].c)) { return FALSE; }
    }

    // Validate indices bounds
    for (int i = 0; i < indices_count; ++i) {
        if (indices[i].a < 0 || indices[i].a >= vertices_count) { return FALSE; }
        if (indices[i].b < 0 || indices[i].b >= vertices_count) { return FALSE; }
        if (indices[i].c < 0 || indices[i].c >= vertices_count) { return FALSE; }
    }

    float unknown_1[3];
    if (!FileBuffer_ReadFloats(buffer, unknown_1, ARRAYSIZE(unknown_1))) { return FALSE; }

    for (int i = 0; i < vertices_count; ++i) {
        float unknown_2[3];
        if (!FileBuffer_ReadFloats(buffer, unknown_2, ARRAYSIZE(unknown_2))) { return FALSE; }
    }

    //
    // Unpack values
    //
    mesh->submesh_count = 1;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeader)); // allocate single submesh
    if (mesh->submeshes == NULL) {
        return FALSE;
    }

    SubmeshHeader *submesh = &mesh->submeshes[0];
    submesh->point_count = vertices_count;
    submesh->points = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshPoint) * vertices_count);
    submesh->indices_count = indices_count;
    submesh->indices = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshTriangle) * indices_count);
    for (int i = 0; i < vertices_count; ++i) {
        submesh->points[i].pos.x = vertices[i].x;
        submesh->points[i].pos.y = vertices[i].y;
        submesh->points[i].pos.z = vertices[i].z;
    }
    for (int i = 0; i < indices_count; ++i) {
        submesh->indices[i].a = indices[i].a;
        submesh->indices[i].b = indices[i].b;
        submesh->indices[i].c = indices[i].c;
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
BOOL Mesh_ReadMethod2(OUT Mesh *mesh, FileBuffer *buffer, const char *file_path)
{
    //
    // Read file
    //
    struct { float x, y, z, u, v; } *vertices;
    struct { short a, b, c; } *indices;

    // Read vertices
    int vertices_count = 0;
    if (!FileBuffer_ReadInt(buffer, &vertices_count)) { return FALSE; }

    DWORD vertices_size = vertices_count * sizeof(vertices[0]);
    if (vertices_size > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check

    vertices = HeapAlloc(GetProcessHeap(), 0, vertices_size);
    for (int i = 0; i < vertices_count; ++i) {
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].x)) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].y)) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].z)) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].u)) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].v)) { return FALSE; }
    }

    // Read indices
    int indices_count = 0;
    if (!FileBuffer_ReadInt(buffer, &indices_count)) { return FALSE; }

    DWORD indices_size = indices_count * sizeof(indices[0]);
    if (indices_size > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check

    indices = HeapAlloc(GetProcessHeap(), 0, indices_size);
    for (int i = 0; i < indices_count; ++i) {
        if (!FileBuffer_ReadShort(buffer, &indices[i].a)) { return FALSE; }
        if (!FileBuffer_ReadShort(buffer, &indices[i].b)) { return FALSE; }
        if (!FileBuffer_ReadShort(buffer, &indices[i].c)) { return FALSE; }
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
    if (!FileBuffer_ReadByte(buffer, &texture_name_length)) { return FALSE; }
    if (!FileBuffer_ReadBytes(buffer, texture_name, texture_name_length)) { return FALSE; }
    texture_name[texture_name_length] = '\0'; // null-terminate string

    float unknown_1[9];
    if (!FileBuffer_ReadFloats(buffer, unknown_1, ARRAYSIZE(unknown_1))) { return FALSE; }

    //
    // Unpack values
    //
    mesh->submesh_count = 1;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeader)); // allocate single submesh
    if (mesh->submeshes == NULL) {
        return FALSE;
    }

    SubmeshHeader *submesh = &mesh->submeshes[0];
    submesh->point_count = vertices_count;
    submesh->points = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshPoint) * vertices_count);
    submesh->indices_count = indices_count;
    submesh->indices = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshTriangle) * indices_count);
    for (int i = 0; i < vertices_count; ++i) {
        submesh->points[i].pos.x = vertices[i].x;
        submesh->points[i].pos.y = vertices[i].y;
        submesh->points[i].pos.z = vertices[i].z;
        submesh->points[i].u = vertices[i].u;
        submesh->points[i].v = vertices[i].v;
    }
    for (int i = 0; i < indices_count; ++i) {
        submesh->indices[i].a = indices[i].a;
        submesh->indices[i].b = indices[i].b;
        submesh->indices[i].c = indices[i].c;
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
BOOL Mesh_ReadMethod5(OUT Mesh *mesh, FileBuffer *buffer, const char *file_path)
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
    if (!FileBuffer_ReadInt(buffer, &unknown_1)) { return FALSE; }
    if (!FileBuffer_ReadInt(buffer, &unknown_2)) { return FALSE; }
    if (!FileBuffer_ReadInt(buffer, &unknown_3)) { return FALSE; }
    if (!FileBuffer_ReadByte(buffer, &unknown_4)) { return FALSE; }
    if (!FileBuffer_ReadByte(buffer, &texture_name_length)) { return FALSE; }
    if (!FileBuffer_ReadChars(buffer, texture_name, texture_name_length)) { return FALSE; }
    texture_name[texture_name_length] = '\0';
    if (!FileBuffer_ReadByte(buffer, &unknown_5)) { return FALSE; }
    if (!FileBuffer_ReadInt(buffer, &vertices_count)) { return FALSE; }
    if (!FileBuffer_ReadInt(buffer, &indices_count)) { return FALSE; }
    if (!FileBuffer_ReadFloats(buffer, unknown_6, ARRAYSIZE(unknown_6))) { return FALSE; }
    if (!FileBuffer_ReadByte(buffer, &unknown_7)) { return FALSE; }
    if (!FileBuffer_ReadFloat(buffer, &unknown_8)) { return FALSE; }
    if (!FileBuffer_ReadFloat(buffer, &unknown_9)) { return FALSE; }
    if (!FileBuffer_ReadFloat(buffer, &unknown_10)) { return FALSE; }

    // Allocate memory
    DWORD vertices_size = vertices_count * sizeof(vertices[0]);
    if (vertices_size > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check
    vertices = HeapAlloc(GetProcessHeap(), 0, vertices_size);
    if (vertices == NULL) { return FALSE; }

    DWORD indices_size = indices_count * sizeof(indices[0]);
    if (indices_size > FileBuffer_GetRemainingSize(buffer)) { return FALSE; } // overflow check
    indices = HeapAlloc(GetProcessHeap(), 0, indices_size);
    if (indices == NULL) { return FALSE; }

    // Read geometry data
    for (int i = 0; i < vertices_count; ++i) {
        if (!FileBuffer_ReadShort(buffer, &vertices[i].x)) { return FALSE; }
        if (!FileBuffer_ReadShort(buffer, &vertices[i].y)) { return FALSE; }
        if (!FileBuffer_ReadShort(buffer, &vertices[i].z)) { return FALSE; }
        if (!FileBuffer_ReadChars(buffer, vertices[i].unknown_1, ARRAYSIZE(vertices[0].unknown_1))) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].u)) { return FALSE; }
        if (!FileBuffer_ReadFloat(buffer, &vertices[i].v)) { return FALSE; }
    }
    for (int i = 0; i < indices_count; ++i) {
        if (!FileBuffer_ReadShort(buffer, &indices[i].a)) { return FALSE; }
        if (!FileBuffer_ReadShort(buffer, &indices[i].b)) { return FALSE; }
        if (!FileBuffer_ReadShort(buffer, &indices[i].c)) { return FALSE; }
    }

    // Validate indices bounds
    for (int i = 0; i < indices_count; ++i) {
        if (indices[i].a < 0 || indices[i].a >= vertices_count) { return FALSE; }
        if (indices[i].b < 0 || indices[i].b >= vertices_count) { return FALSE; }
        if (indices[i].c < 0 || indices[i].c >= vertices_count) { return FALSE; }
    }

    if (!FileBuffer_ReadFloat(buffer, &unknown_11)) { return FALSE; }
    if (!FileBuffer_ReadFloat(buffer, &unknown_12)) { return FALSE; }
    if (!FileBuffer_ReadFloat(buffer, &unknown_13)) { return FALSE; }
    if (!FileBuffer_ReadFloats(buffer, unknown_14, ARRAYSIZE(unknown_14))) { return FALSE; }

    //
    // Unpack values
    //
    mesh->submesh_count = 1;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshHeader)); // allocate single submesh
    if (mesh->submeshes == NULL) {
        return FALSE;
    }

    SubmeshHeader *submesh = &mesh->submeshes[0];
    submesh->point_count = vertices_count;
    submesh->points = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshPoint) * vertices_count);
    submesh->indices_count = indices_count;
    submesh->indices = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshTriangle) * indices_count);
    for (int i = 0; i < vertices_count; ++i) {
        // submesh->points[i].pos.x = (float)vertices[i].x / (float)INT16_MAX; // unpack coordinates
        // submesh->points[i].pos.y = (float)vertices[i].y / (float)INT16_MAX;
        // submesh->points[i].pos.z = (float)vertices[i].z / (float)INT16_MAX;
        submesh->points[i].pos.x = (float)vertices[i].x; // unpack coordinates
        submesh->points[i].pos.y = (float)vertices[i].y;
        submesh->points[i].pos.z = (float)vertices[i].z;
        submesh->points[i].u = vertices[i].u;
        submesh->points[i].v = vertices[i].v;
    }
    for (int i = 0; i < indices_count; ++i) {
        submesh->indices[i].a = indices[i].a;
        submesh->indices[i].b = indices[i].b;
        submesh->indices[i].c = indices[i].c;
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

BOOL Uncompress(OUT BYTE **uncompressed_data, OUT DWORD *uncompressed_data_size, BYTE *compressed_data, DWORD compressed_data_size)
{
    size_t decoded_size = compressed_data_size * 4;
    BYTE *decoded = malloc(decoded_size);
    if (!decoded) {
        return FALSE;
    }

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = compressed_data;
    stream.avail_in = compressed_data_size;

    if (inflateInit2(&stream, 15 + 32) != Z_OK) {
        free(decoded);
        return FALSE;
    }

    int status;
    do {
        stream.next_out = decoded + stream.total_out;
        stream.avail_out = decoded_size - stream.total_out;

        status = inflate(&stream, Z_NO_FLUSH);

        if (status == Z_BUF_ERROR && stream.avail_out == 0) {
            size_t new_size = decoded_size * 2;
            BYTE *new_decoded = realloc(decoded, new_size);
            if (!new_decoded) {
                inflateEnd(&stream);
                free(decoded);
                return FALSE;
            }
            decoded = new_decoded;
            decoded_size = new_size;
        }
    } while (status == Z_OK || status == Z_BUF_ERROR);

    inflateEnd(&stream);

    if (status != Z_STREAM_END) {
        free(decoded);
        return FALSE;
    }

    *uncompressed_data = decoded;
    *uncompressed_data_size = stream.total_out;
    return TRUE;
}

BOOL Mesh_Read(OUT Mesh *mesh, HANDLE file, const char *file_path)
{
    DWORD file_size = GetFileSize(file, NULL);
    if (file_size == INVALID_FILE_SIZE) {
        return FALSE;
    }

    // Allocate memory and read whole file
    void *file_data = HeapAlloc(GetProcessHeap(), 0, file_size);
    if (file_data == NULL) {
        return FALSE;
    }
    DWORD bytes_read;
    if (!ReadFile(file, file_data, file_size, &bytes_read, NULL) || bytes_read != file_size) {
        HeapFree(GetProcessHeap(), 0, file_data);
        return FALSE;
    }

    FileBuffer buffer;
    buffer.data = file_data;
    buffer.size = file_size;
    buffer.offset = 0;

    // Depending on first int value determine the mesh type
    int first_value;
    if (!FileBuffer_ReadInt(&buffer, &first_value)) {
        HeapFree(GetProcessHeap(), 0, file_data);
        return FALSE;
    }
    BOOL with_subblocks = FALSE;
    if (first_value == 0) {
        with_subblocks = TRUE;
    } else {
        WORD second_value;
        if (!FileBuffer_ReadWord(&buffer, &second_value)) {
            HeapFree(GetProcessHeap(), 0, file_data);
            return FALSE;
        }
        if (second_value == 55928) { // 0x78DA
            // compressed data
            // BYTE *uncompressed_data = NULL;
            // DWORD uncompressed_data_size = 0;
            // Uncompress(&uncompressed_data, &uncompressed_data_size, file_data + 4, file_size - 4);
            // buffer.data = uncompressed_data;
            // buffer.size = uncompressed_data_size;
            // buffer.offset = 0;
            return FALSE; // not supported yet
        }
    }

    BOOL success = FALSE;
    if (!success) {
        // If previous method failed, reset file pointer and try method 1
        buffer.offset = 0;
        success = Mesh_ReadStructureType3(mesh, &buffer, file_path, with_subblocks);
    }
    if (!success) {
        // If previous method failed, reset file pointer and try method 1
        buffer.offset = 0;
        success = Mesh_ReadMethod4(mesh, &buffer, file_path); // try method 4
    }
    if (!success) {
        // If previous method failed, reset file pointer and try method 1
        buffer.offset = 0;
        success = Mesh_ReadMethod1(mesh, &buffer, file_path);
    }
    if (!success) {
        // If previous method failed, reset file pointer and try method 2
        buffer.offset = 0;
        success = Mesh_ReadMethod2(mesh, &buffer, file_path);
    }
    if (!success) {
        // If previous method failed, reset file pointer and try method 5
        buffer.offset = 0;
        success = Mesh_ReadMethod5(mesh, &buffer, file_path);
    }
    return success;
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
