#include "mesh.h"

#include <shlwapi.h>
#include <stdint.h>

//
// Notes:
// - Seems like .mesh file contains submesh blocks each separated by different material
// - Somewhere in unknown values should be collision data or AABB rect
//

typedef struct {
    int unknown_4;
    int unknown_5;
    int unknown_array_length_1;
    int unknown_array_length_2;
    int unknown_array_length_3;
    int unknown_array_length_4;
    float unknown_13;
} SubmeshUnknownBlockSubblock;

typedef struct {
    int unknown_2;
    float unknown_15floats[15];
    int subblocks_count;
    SubmeshUnknownBlockSubblock *subblocks;
} SubmeshUnknownBlock;

typedef struct {
    int unknown_1; // id?
    int unknown_2; // probably material id, affects the following structure type
    char unknown_3; // unknown_2 != 5
    union {
        struct {
            float unknown_4[6];
        } type1; // unknown_2 == 0
        struct {
            unsigned char texture_name_length;
            char texture_name[255];
        } type2;// unknown_2 != 0
    };

    int point_count;
    int indices_count;
    int unknown_blocks_count;
    SubmeshUnknownBlock *unknown_blocks;

} SubmeshInfoBlock;

BOOL Mesh_Read(OUT Mesh *mesh, HANDLE file, const char *path)
{
    DWORD file_size = GetFileSize(file, NULL);

    // Depending on first int value determine the mesh type
    int mesh_type = 0;

    int first_value;
    if (!ReadFile(file, &first_value, sizeof(int), NULL, NULL)) { return FALSE; }
    if (HIBYTE(LOWORD(first_value))) { // "pz  " - ZIP compression
        return FALSE; // TODO: compressed models not supported yet
    }
    if (first_value == 0) {
        // read value again
        mesh_type = 1;
        if (!ReadFile(file, &first_value, sizeof(int), NULL, NULL)) { return FALSE; }
    }

    int submesh_count = first_value;

    mesh->submesh_count = submesh_count;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshSubmesh) * submesh_count);

    SubmeshInfoBlock *submesh_info_blocks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshInfoBlock) * submesh_count);
    for (int i = 0; i < submesh_count; ++i) {
        SubmeshInfoBlock *submesh_info = &submesh_info_blocks[i];

        // ??? (probably ID)
        if (!ReadFile(file, &submesh_info->unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &submesh_info->unknown_2, sizeof(int), NULL, NULL)) { return FALSE; }
        if (submesh_info->unknown_2 != 5) {
            if (!ReadFile(file, &submesh_info->unknown_3, sizeof(char), NULL, NULL)) { return FALSE; }
        }
        if (submesh_info->unknown_2 == 0) {
            // ??? unknown 6 floats
            if (!ReadFile(file, &submesh_info->type1.unknown_4, sizeof(float) * 6, NULL, NULL)) { return FALSE; }
        } else {
            // Read texture name
            if (!ReadFile(file, &submesh_info->type2.texture_name_length, sizeof(char), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, submesh_info->type2.texture_name, submesh_info->type2.texture_name_length, NULL, NULL)) { return FALSE; }
            submesh_info->type2.texture_name[submesh_info->type2.texture_name_length] = '\0';

            // ???
            if (submesh_info->unknown_2 != 17) {
                char unknown_4;
                if (!ReadFile(file, &unknown_4, sizeof(char), NULL, NULL)) { return FALSE; }
            }
            if (submesh_info->unknown_2 & 0b00000100) {
                float unknown_4floats[4];
                if (!ReadFile(file, unknown_4floats, sizeof(float) * 4, NULL, NULL)) { return FALSE; }
            }
        }

        // Read vertices and indices count
        if (!ReadFile(file, &submesh_info->point_count, sizeof(int), NULL, NULL)) { return FALSE; }
        if (!ReadFile(file, &submesh_info->indices_count, sizeof(int), NULL, NULL)) { return FALSE; }

        // Sanity checks
        if ((submesh_info->point_count * sizeof(int)) > file_size) { return FALSE; }
        if ((submesh_info->indices_count * sizeof(int)) > file_size) { return FALSE; }

        // ???
        float unknown_19floats[19];
        if (!ReadFile(file, unknown_19floats, sizeof(float) * 19, NULL, NULL)) { return FALSE; }

        // ?? unknown blocks that exists only for mesh_type=1
        if (mesh_type == 1) {
            int unknown_blocks_count;
            if (!ReadFile(file, &unknown_blocks_count, sizeof(int), NULL, NULL)) { return FALSE; }
            // Sanity check
            if ((unknown_blocks_count * sizeof(SubmeshUnknownBlock)) > file_size) { return FALSE; }

            submesh_info->unknown_blocks_count = unknown_blocks_count;
            if (unknown_blocks_count > 0) {
                submesh_info->unknown_blocks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshUnknownBlock) * unknown_blocks_count);
                for (int j = 0; j < unknown_blocks_count; ++j) {
                    SubmeshUnknownBlock *unknown_block = &submesh_info->unknown_blocks[j];

                    if (!ReadFile(file, &unknown_block->unknown_2, sizeof(int), NULL, NULL)) { return FALSE; }
                    if (!ReadFile(file, unknown_block->unknown_15floats, sizeof(float) * 15, NULL, NULL)) { return FALSE; }
                    if (!ReadFile(file, &unknown_block->subblocks_count, sizeof(int), NULL, NULL)) { return FALSE; }
                    if (unknown_block->subblocks_count != 0) {
                        unknown_block->subblocks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SubmeshUnknownBlockSubblock) * unknown_block->subblocks_count);

                        for (int k = 0; k < unknown_block->subblocks_count; ++k) {
                            SubmeshUnknownBlockSubblock *subblock = &unknown_block->subblocks[k];

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
    }

    // Load textures
    for (int i = 0; i < submesh_count; ++i) {
        SubmeshInfoBlock *block = &submesh_info_blocks[i];

        if (block->unknown_2 != 0) {

            // Find texture path
            char texture_path[MAX_PATH];
            strcpy(texture_path, path); // copy .mesh path
            if (!PathRemoveFileSpec(texture_path)) { return FALSE; } // remove file
            if (!PathRemoveFileSpec(texture_path)) { return FALSE; } // go up dir
            if (PathCombine(texture_path, texture_path, "Textures") == NULL) { return FALSE; } // go to "Textures" directory
            if (PathCombine(texture_path, texture_path, block->type2.texture_name) == NULL) { return FALSE; } // add texture name

            // Load texture
            MeshSubmesh *submesh = &mesh->submeshes[i];
            if (!Texture_Load(&submesh->texture, texture_path)) {
                TCHAR message[1000];
                wsprintf(message, "\"%s\" not found. Please extract \"Textures.vfs\" archive", texture_path);
                MessageBox(NULL, message, "Texture not found", MB_OK | MB_ICONWARNING);
                return FALSE;
            }
        }
    }

    // Read blocks data
    for (int i = 0; i < submesh_count; ++i) {

        // ??? (probably some coordinates)
        char unknown_byte;
        if (!ReadFile(file, &unknown_byte, sizeof(char), NULL, NULL)) { return FALSE; }
        float unknown_float1;
        if (!ReadFile(file, &unknown_float1, sizeof(float), NULL, NULL)) { return FALSE; }
        float unknown_float2;
        if (!ReadFile(file, &unknown_float2, sizeof(float), NULL, NULL)) { return FALSE; }
        float unknown_float3;
        if (!ReadFile(file, &unknown_float3, sizeof(float), NULL, NULL)) { return FALSE; }

        SubmeshInfoBlock *block = &submesh_info_blocks[i];
        MeshSubmesh *submesh = &mesh->submeshes[i];

        // Read points
        submesh->point_count = block->point_count;
        submesh->points = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshPoint) * submesh->point_count);
        for (int j = 0; j < block->point_count; ++j) {
            MeshPoint *point = &submesh->points[j];

            short x, y, z;
            if (!ReadFile(file, &x, sizeof(short), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &y, sizeof(short), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &z, sizeof(short), NULL, NULL)) { return FALSE; }

            point->x = (float)x / INT16_MAX;
            point->y = (float)y / INT16_MAX;
            point->z = (float)z / INT16_MAX;

            int unknown_1;
            if (!ReadFile(file, &unknown_1, sizeof(int), NULL, NULL)) { return FALSE; }

            if (block->unknown_2 & 0b00000001) {
                float u, v;
                if (!ReadFile(file, &u, sizeof(float), NULL, NULL)) { return FALSE; }
                if (!ReadFile(file, &v, sizeof(float), NULL, NULL)) { return FALSE; }
                point->u = u;
                point->v = v;
            }
            if (block->unknown_2 != 5 && block->unknown_3 != 0) {
                char unknown_2[4];
                if (!ReadFile(file, &unknown_2, sizeof(char) * 4, NULL, NULL)) { return FALSE; }
            }
        }

        // Read triangles
        submesh->triangle_count = block->indices_count;
        submesh->triangles = HeapAlloc(GetProcessHeap(), 0, sizeof(MeshTriangle) * submesh->triangle_count);
        for (int j = 0; j < block->indices_count; ++j) {
            MeshTriangle *triangle = &submesh->triangles[j];

            // Read indices
            unsigned short v1, v2, v3;
            if (!ReadFile(file, &v1, sizeof(unsigned short), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &v2, sizeof(unsigned short), NULL, NULL)) { return FALSE; }
            if (!ReadFile(file, &v3, sizeof(unsigned short), NULL, NULL)) { return FALSE; }

            // Sanity checks
            if (v1 >= submesh->point_count) { return FALSE; }
            if (v2 >= submesh->point_count) { return FALSE; }
            if (v3 >= submesh->point_count) { return FALSE; }

            triangle->a = v1;
            triangle->b = v2;
            triangle->c = v3;
        }

        // ???
        for (int i2 = 0; i2 < block->unknown_blocks_count; ++i2) {
            SubmeshUnknownBlock *unknown_block = &block->unknown_blocks[i2];

            for (int i3 = 0; i3 < unknown_block->subblocks_count; ++i3) {
                SubmeshUnknownBlockSubblock *subblock = &unknown_block->subblocks[i3];

                for (int i4 = 0; i4 < subblock->unknown_array_length_3; ++i4) {
                    unsigned short unknown_short;
                    if (!ReadFile(file, &unknown_short, sizeof(short), NULL, NULL)) { return FALSE; }
                    float unknown_float4;
                    if (!ReadFile(file, &unknown_float4, sizeof(float), NULL, NULL)) { return FALSE; }
                    float unknown_float5;
                    if (!ReadFile(file, &unknown_float5, sizeof(float), NULL, NULL)) { return FALSE; }
                }
            }
        }
    }

    return TRUE;
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
