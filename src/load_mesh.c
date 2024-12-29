#include "mesh.h"

#include <shlwapi.h>
#include <stdint.h>

//
// Notes:
// - Seems like .mesh file contains submesh blocks each separated by different material
// - Somewhere in unknown values should be collision data or AABB rect
//

typedef struct {
    int unknown_10;
} SubmeshUnknownBlock;

typedef struct {
    int unknown_1;
    int unknown_2;
    char unknown_3;
    unsigned char texture_name_length;
    char texture_name[255];
    char unknown_5; // probably null terminator or "texture has alpha channel" flag
    int point_count;
    int indices_count;

    int unknown_blocks_count;
    SubmeshUnknownBlock *unknown_blocks;

} SubmeshInfoBlock;

BOOL Mesh_Read(OUT Mesh *mesh, HANDLE file, const char *path, BOOL float_vertices)
{
    DWORD file_size = GetFileSize(file, NULL);

    // Determine texture count
    int submesh_count;
    if (!ReadFile(file, &submesh_count, sizeof(int), NULL, NULL)) {
        return FALSE;
    }

    int mesh_type = 0;
    if (submesh_count == 0) {
        if (!ReadFile(file, &submesh_count, sizeof(int), NULL, NULL)) {
            return FALSE;
        }
        mesh_type = 1;
    }

    mesh->submesh_count = submesh_count;
    mesh->submeshes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MeshSubmesh) * submesh_count);

    SubmeshInfoBlock *submesh_info_blocks = HeapAlloc(GetProcessHeap(), 0, sizeof(SubmeshInfoBlock) * submesh_count);
    for (int i = 0; i < submesh_count; ++i) {
        SubmeshInfoBlock *submesh_info = &submesh_info_blocks[i];

        // ???
        if (!ReadFile(file, &submesh_info->unknown_1, sizeof(int), NULL, NULL)) { // probably ID
            return FALSE;
        }
        if (!ReadFile(file, &submesh_info->unknown_2, sizeof(int), NULL, NULL)) { // unknown, usually 1 or 17 (maybe material ID?)
            return FALSE;
        }
        if (submesh_info->unknown_2 != 0) {
            if (!ReadFile(file, &submesh_info->unknown_3, sizeof(char), NULL, NULL)) {
                return FALSE;
            }
        }

        // Read texture name
        if (!ReadFile(file, &submesh_info->texture_name_length, sizeof(char), NULL, NULL)) {
            return FALSE;
        }
        if (!ReadFile(file, submesh_info->texture_name, submesh_info->texture_name_length, NULL, NULL)) {
            return FALSE;
        }
        submesh_info->texture_name[submesh_info->texture_name_length] = '\0';

        // ???
        if (submesh_info->unknown_2 == 1) {
            char unknown_4;
            if (!ReadFile(file, &unknown_4, sizeof(char), NULL, NULL)) {
                return FALSE;
            }
        }
        if (submesh_info->unknown_2 == 5) {
            float unknown_4floats[4];
            if (!ReadFile(file, unknown_4floats, sizeof(float) * 4, NULL, NULL)) {
                return FALSE;
            }
        }

        // Read vertices and indices count
        if (!ReadFile(file, &submesh_info->point_count, sizeof(int), NULL, NULL)) {
            return FALSE;
        }
        if (!ReadFile(file, &submesh_info->indices_count, sizeof(int), NULL, NULL)) {
            return FALSE;
        }

        // Sanity checks
        if ((submesh_info->point_count * sizeof(int)) > file_size) {
            return FALSE;
        }
        if ((submesh_info->indices_count * sizeof(int)) > file_size) {
            return FALSE;
        }

        // ???
        float unknown_19floats[19];
        if (!ReadFile(file, unknown_19floats, sizeof(float) * 19, NULL, NULL)) {
            return FALSE;
        }

        if (mesh_type == 1) {
            // ???
            int unknown_blocks_count;
            if (!ReadFile(file, &unknown_blocks_count, sizeof(int), NULL, NULL)) {
                return FALSE;
            }
            // Sanity check
            if ((unknown_blocks_count * ((sizeof(int) * 8) + (sizeof(float) * 16))) > file_size) {
                return FALSE;
            }

            submesh_info->unknown_blocks_count = unknown_blocks_count;
            submesh_info->unknown_blocks = HeapAlloc(GetProcessHeap(), 0, sizeof(SubmeshUnknownBlock) * unknown_blocks_count);
            for (int j = 0; j < unknown_blocks_count; ++j) {
                SubmeshUnknownBlock *unknown_block = &submesh_info->unknown_blocks[j];

                int unknown_2;
                if (!ReadFile(file, &unknown_2, sizeof(int), NULL, NULL)) {
                    return FALSE;
                }
                float unknown_15floats[15];
                if (!ReadFile(file, unknown_15floats, sizeof(float) * 15, NULL, NULL)) {
                    return FALSE;
                }
                int unknown_3;
                if (!ReadFile(file, &unknown_3, sizeof(int), NULL, NULL)) {
                    return FALSE;
                }
                if (unknown_3 == 1) {
                    int unknown_4;
                    if (!ReadFile(file, &unknown_4, sizeof(int), NULL, NULL)) {
                        return FALSE;
                    }

                    int unknown_5;
                    if (!ReadFile(file, &unknown_5, sizeof(int), NULL, NULL)) {
                        return FALSE;
                    }

                    int unknown_6;
                    if (!ReadFile(file, &unknown_6, sizeof(int), NULL, NULL)) {
                        return FALSE;
                    }
                    SetFilePointer(file, sizeof(char) * unknown_6, NULL, FILE_CURRENT); // skip

                    int unknown_8;
                    if (!ReadFile(file, &unknown_8, sizeof(int), NULL, NULL)) {
                        return FALSE;
                    }
                    SetFilePointer(file, sizeof(char) * unknown_8, NULL, FILE_CURRENT); // skip

                    int unknown_10;
                    if (!ReadFile(file, &unknown_10, sizeof(int), NULL, NULL)) {
                        return FALSE;
                    }
                    unknown_block->unknown_10 = unknown_10;

                    int unknown_11;
                    if (!ReadFile(file, &unknown_11, sizeof(int), NULL, NULL)) {
                        return FALSE;
                    }
                    SetFilePointer(file, sizeof(short) * 4 * unknown_11, NULL, FILE_CURRENT); // skip

                    float unknown_13;
                    if (!ReadFile(file, &unknown_13, sizeof(float), NULL, NULL)) {
                        return FALSE;
                    }
                }
            }
        }
    }

    // Load textures
    for (int i = 0; i < submesh_count; ++i) {
        SubmeshInfoBlock *block = &submesh_info_blocks[i];

        // Load texture
        char texture_path[MAX_PATH];
        strcpy(texture_path, path); // copy .mesh path
        if (!PathRemoveFileSpec(texture_path)) { // remove file
            return FALSE;
        }
        if (!PathRemoveFileSpec(texture_path)) { // go up dir
            return FALSE;
        }
        if (PathCombine(texture_path, texture_path, "Textures") == NULL) { // go to "Textures" directory
            return FALSE;
        }
        if (PathCombine(texture_path, texture_path, block->texture_name) == NULL) { // add texture name
            return FALSE;
        }

        MeshSubmesh *submesh = &mesh->submeshes[i];
        if (!Texture_Load(&submesh->texture, texture_path)) {
            TCHAR message[1000];
            wsprintf(message, "\"%s\" not found. Please extract \"Textures.vfs\" archive", texture_path);
            MessageBox(NULL, message, "Texture not found", MB_OK | MB_ICONWARNING);
            return FALSE;
        }
    }

    // Read blocks data
    for (int i = 0; i < submesh_count; ++i) {
    // for (int i = 0; i < 1; ++i) {

        // ???
        char unknown_byte;
        if (!ReadFile(file, &unknown_byte, sizeof(char), NULL, NULL)) {
            return FALSE;
        }
        float unknown_float1;
        if (!ReadFile(file, &unknown_float1, sizeof(float), NULL, NULL)) {
            return FALSE;
        }
        float unknown_float2;
        if (!ReadFile(file, &unknown_float2, sizeof(float), NULL, NULL)) {
            return FALSE;
        }
        float unknown_float3;
        if (!ReadFile(file, &unknown_float3, sizeof(float), NULL, NULL)) {
            return FALSE;
        }

        SubmeshInfoBlock *block = &submesh_info_blocks[i];
        MeshSubmesh *submesh = &mesh->submeshes[i];

        // Read points
        submesh->point_count = block->point_count;
        submesh->points = HeapAlloc(GetProcessHeap(), 0, sizeof(MeshPoint) * submesh->point_count);
        for (int j = 0; j < block->point_count; ++j) {
            MeshPoint *point = &submesh->points[j];

            if (float_vertices) {
                float x;
                if (!ReadFile(file, &x, sizeof(float), NULL, NULL)) {
                    return FALSE;
                }
                float y;
                if (!ReadFile(file, &y, sizeof(float), NULL, NULL)) {
                    return FALSE;
                }
                float z;
                if (!ReadFile(file, &z, sizeof(float), NULL, NULL)) {
                    return FALSE;
                }

                // skip unknown 5 floats
                float unknown_5floats;
                if (!ReadFile(file, &unknown_5floats, sizeof(float) * 5, NULL, NULL)) {
                    return FALSE;
                }

                point->x = x;
                point->y = y;
                point->z = z;

            } else {
                short x;
                if (!ReadFile(file, &x, sizeof(short), NULL, NULL)) {
                    return FALSE;
                }
                short y;
                if (!ReadFile(file, &y, sizeof(short), NULL, NULL)) {
                    return FALSE;
                }
                short z;
                if (!ReadFile(file, &z, sizeof(short), NULL, NULL)) {
                    return FALSE;
                }

                point->x = (float)x / INT16_MAX;
                point->y = (float)y / INT16_MAX;
                point->z = (float)z / INT16_MAX;

                // skip unknown 4 bytes
                char unknown_4bytes[4];
                if (!ReadFile(file, unknown_4bytes, sizeof(char) * 4, NULL, NULL)) {
                    return FALSE;
                }

                float u;
                if (!ReadFile(file, &u, sizeof(float), NULL, NULL)) {
                    return FALSE;
                }
                float v;
                if (!ReadFile(file, &v, sizeof(float), NULL, NULL)) {
                    return FALSE;
                }

                point->u = u;
                point->v = v;
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
        for (int j = 0; j < block->unknown_blocks_count; ++j) {
            SubmeshUnknownBlock *unknown_block = &block->unknown_blocks[j];
            for (int k = 0; k < unknown_block->unknown_10; ++k) {
                unsigned short unknown_short;
                if (!ReadFile(file, &unknown_short, sizeof(short), NULL, NULL)) {
                    return FALSE;
                }
                float unknown_float4;
                if (!ReadFile(file, &unknown_float4, sizeof(float), NULL, NULL)) {
                    return FALSE;
                }
                float unknown_float5;
                if (!ReadFile(file, &unknown_float5, sizeof(float), NULL, NULL)) {
                    return FALSE;
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
    BOOL success = Mesh_Read(mesh, file, path, FALSE);
    if (!success) {
        ZeroMemory(mesh, sizeof(Mesh));
    }

    CloseHandle(file);

    return success;
}
