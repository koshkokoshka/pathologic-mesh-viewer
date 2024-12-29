#include "mesh.h"

#include <shlwapi.h>
#include <stdint.h>

//
// Notes:
// - Seems like .mesh file contains submesh blocks each separated by different material
// - Somewhere in unknown values should be collision data or AABB rect
//

typedef struct {
    int unknown_1;
    int unknown_2;
    char unknown_3;
    unsigned char texture_name_length;
    char texture_name[255];
    char unknown_5; // probably null terminator or "texture has alpha channel" flag
    int point_count;
    int indices_count;
} MeshFileTextureBlock;

BOOL Mesh_Read(OUT Mesh *mesh, HANDLE file, const char *path, BOOL float_vertices)
{
    DWORD file_size = GetFileSize(file, NULL);

    // Determine texture count
    int texture_count;
    if (!ReadFile(file, &texture_count, sizeof(int), NULL, NULL)) {
        return FALSE;
    }

    int mesh_type = 0;
    if (texture_count == 0) {
        if (!ReadFile(file, &texture_count, sizeof(int), NULL, NULL)) {
            return FALSE;
        }
        mesh_type = 1;
    }

    mesh->texture_count = texture_count;
    mesh->textures = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Texture) * texture_count);

    MeshFileTextureBlock *texture_blocks = HeapAlloc(GetProcessHeap(), 0, sizeof(MeshFileTextureBlock) * texture_count);

    for (int i = 0; i < texture_count; ++i) {
        MeshFileTextureBlock *block = &texture_blocks[i];

        // ???
        if (!ReadFile(file, &block->unknown_1, sizeof(int), NULL, NULL)) {
            return FALSE;
        }
        if (!ReadFile(file, &block->unknown_2, sizeof(int), NULL, NULL)) {
            return FALSE;
        }
        if (block->unknown_2 == 1) {
            if (!ReadFile(file, &block->unknown_3, sizeof(char), NULL, NULL)) {
                return FALSE;
            }
        }

        // Read texture name
        if (!ReadFile(file, &block->texture_name_length, sizeof(char), NULL, NULL)) {
            return FALSE;
        }
        if (!ReadFile(file, block->texture_name, block->texture_name_length, NULL, NULL)) {
            return FALSE;
        }
        block->texture_name[block->texture_name_length] = '\0';

        // ???
        if (!ReadFile(file, &block->unknown_5, sizeof(char), NULL, NULL)) { // probably null terminator
            return FALSE;
        }

        if (block->unknown_2 == 5) {
            float unknown_4floats[4];
            if (!ReadFile(file, unknown_4floats, sizeof(float) * 4, NULL, NULL)) {
                return FALSE;
            }
        }

        // Read vertices and indices count
        if (!ReadFile(file, &block->point_count, sizeof(int), NULL, NULL)) {
            return FALSE;
        }
        if (!ReadFile(file, &block->indices_count, sizeof(int), NULL, NULL)) {
            return FALSE;
        }

        // Sanity checks
        if ((block->point_count * sizeof(int)) > file_size) {
            return FALSE;
        }
        if ((block->indices_count * sizeof(int)) > file_size) {
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
            for (int j = 0; j < unknown_blocks_count; ++j) {
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

    // ???
    char unknown_13bytes[13];
    if (!ReadFile(file, unknown_13bytes, sizeof(char) * 13, NULL, NULL)) {
        return FALSE;
    }

    // Load textures
    for (int i = 0; i < texture_count; ++i) {
        MeshFileTextureBlock *block = &texture_blocks[i];

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
        if (!Texture_Load(&mesh->textures[i], texture_path)) {
            TCHAR message[1000];
            wsprintf(message, "\"%s\" not found. Please extract \"Textures.vfs\" archive", texture_path);
            MessageBox(NULL, message, "Texture not found", MB_OK | MB_ICONWARNING);
            return FALSE;
        }
    }

    // Read blocks data
//    for (int i = 0; i < texture_count; ++i) {
    for (int i = 0; i < 1; ++i) {
        MeshFileTextureBlock *block = &texture_blocks[i];

        // Read points
        mesh->point_count = block->point_count;
        mesh->points = HeapAlloc(GetProcessHeap(), 0, sizeof(MeshPoint) * mesh->point_count);
        for (int j = 0; j < block->point_count; ++j) {
            MeshPoint *point = &mesh->points[j];

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

//    if (skip_unknown_gap) {
//        cursor += sizeof(char); // ???
//    }

        // Read triangles
        mesh->triangle_count = block->indices_count;
        mesh->triangles = HeapAlloc(GetProcessHeap(), 0, sizeof(MeshTriangle) * mesh->triangle_count);
        for (int j = 0; j < block->indices_count; ++j) {
            MeshTriangle *triangle = &mesh->triangles[j];

            unsigned short v1;
            if (!ReadFile(file, &v1, sizeof(unsigned short), NULL, NULL)) {
                return FALSE;
            }
            unsigned short v2;
            if (!ReadFile(file, &v2, sizeof(unsigned short), NULL, NULL)) {
                return FALSE;
            }
            unsigned short v3;
            if (!ReadFile(file, &v3, sizeof(unsigned short), NULL, NULL)) {
                return FALSE;
            }

            // Sanity checks
            if (v1 >= mesh->point_count) { return FALSE; }
            if (v2 >= mesh->point_count) { return FALSE; }
            if (v3 >= mesh->point_count) { return FALSE; }

            triangle->a = v1;
            triangle->b = v2;
            triangle->c = v3;
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
