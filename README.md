# Pathologic Mesh Viewer

A 3D model viewer for Pathologic Classic HD that supports the .mesh file format used in the game.

![screenshot](screenshot.png)

> **Note:** Pathologic Mesh Viewer is still under development and may not fully support every `.mesh` file from the game.

## How to Use

1. **Download or Build the Viewer**
    - [Download a prebuilt version](https://github.com/koshkokoshka/pathologic-mesh-viewer/releases) or build the project from the source code.

2. **Prepare the Game Assets**
    - Extract `Geometries.vfs` and `Textures.vfs` archives located in game data folder.

3. **Open a Model**
    - Use Pathologic Mesh Viewer to open any `.mesh` file from the extracted assets.

---

## Specification

The goal of this project is to fully reverse-engineer the .mesh file structure used in the Pathologic game and its alpha version, to give the modding community ability to extract, modify, and reimport game assets.

### .mesh file structure (NOT COMPLETE)

See [`src/load_mesh.c`](https://github.com/koshkokoshka/pathologic-mesh-viewer/blob/master/src/load_mesh.c) for details

The following structure is a .bt template for the [SweetScape 010 Editor](https://www.sweetscape.com/010editor/):

```c++
struct {
    // The .mesh file structure is inconsistent, and I haven't yet figured out how to reliably identify the correct structure.
    // For now, I use different approaches for different meshes, hoping to eventually combine this knowledge into a unified file structure.
    //
    // structure_type=0 - pond_water.mesh
    // structure_type=1 - leaves01.mesh
    // structure_type=2 - imnogogrannik.mesh
    // structure_type=3 - Bench01.mesh
    // structure_type=4 - iboiny.mesh, ihidden_room.mesh
    // structure_type=5 - pulya6.mesh
    local int structure_type = 4;

    if (structure_type == 0) {
        int vertices_count;
        struct {
            float x, y, z;
        } vertices[vertices_count];
        int indices_count;
        struct { short a, b, c; } indices[indices_count];
        float unknown_1[3];
        float unknown_2[3 * vertices_count];     
    }
 
    if (structure_type == 1) {
        int vertices_count;
        struct {
            float x, y, z;
            float u, v;
        } vertices[vertices_count];
        int indices_count;
        struct { short a, b, c; } indices[indices_count];
        char texture_name_length;
        char texture_name[texture_name_length];
        float unknown_2[9];
    }
 
    if (structure_type == 2) {
        int unknown_1;
        struct {
            byte unknown_1;
            float unknown_2;
            float unknown_3;
            float unknown_4;
            float unknown_5;
        } unknown_2[6];
        char unknown_3[9];
        float unknown_4[3];
        int unknown_5;
        byte unknown_6;
        int unknown_7_length;
        int unknown_7[unknown_7_length];
        byte unknown_8[12];
    }
 
    if (structure_type == 3) {
        int unknown_1;
        int submesh_count;
        struct {
            // material_type=33 - isobor_lampa.mesh
            // material_type=33 - icot_eva_Kerosinka.mesh
            // material_type=17 - inv_grass_white_plet.mesh
            // material_type=17 - r4_house3_01.mesh
            // material_type=5 - Sobor.mesh
            int unknown_1;
            int material_type; // probably material flags
            if (material_type == 0) {
                char unknown_3;
                float unknown_4[6];
            } else {
                if (material_type == 1 || material_type == 17) {
                    char unknown_3;
                }
                char texture_name_length;
                char texture_name[texture_name_length];
                
                if (material_type == 1 || material_type == 33) {
                    char unknown_4;
                }
                if (material_type == 5) {
                    struct {
                        char unknown_4;
                        float unknown_5[4];
                    } unknown_4;
                }
            }
    
            int vertices_count;
            int indices_count;
    
            float unknown_5[19]; // Sphere3D and COBB3D        
    
            if (material_type == 0 || material_type == 1 || material_type == 5 || material_type == 17) {
                int unknown_block_count;
                local int i;
                for (i = 0; i < unknown_block_count; i++) {
                    struct {
                        int unknown_1; // probably id
                        float unknown_2[15];
                        int subblocks_count;
                        if (subblocks_count > 0) {
                            struct {
                                int unknown_4;
                                int unknown_5;
                                int unknown_array_length_1;
                                char unknown_7[unknown_array_length_1];
                                int unknown_array_length_2;
                                char unknown_9[unknown_array_length_2];
                                int unknown_array_length_3;
                                int unknown_array_length_4;
                                struct { short a, b, c, d; } unknown12[unknown_array_length_4];
                                float unknown_13;
                            } subblocks[subblocks_count] <optimize=false>;
                        }
                    } unknown_block;
                }
            }
        } submeshes[submesh_count] <optimize=false>;
    
        local int i;
        for (i = 0; i < submesh_count; i++) {
            struct {
                byte vertices_type; // 0 - float, 1 - short
                if (vertices_type == 1) {
                    struct {
                        float unknown2; // probably world coordinates
                        float unknown3;
                        float unknown4;
                    } unknown_1;
                }
    
                struct {
                    if (vertices_type == 0) {
                        float x;
                        float y;
                        float z;
                    } else {
                        short x;
                        short y;
                        short z;
                    }
                    
                    if (submeshes[i].material_type == 0) { // ihouse2_krovatka01.mesh
                        int unknown_6;
                        if (submeshes[i].unknown_3 == 1) { // iboiny_railing01.mesh
                            int unknown_6;                        
                        }
                    }
                    if (submeshes[i].material_type == 1) {
                        int unknown_1;
                        float u, v;
                        if (submeshes[i].unknown_3 == 1) { // iboiny_railing01.mesh
                            int unknown_6;
                        }
                    }
                    if (submeshes[i].material_type == 5) { // Sobor.mesh
                        int unknown_1;
                        float u, v;
                    }
                    if (submeshes[i].material_type == 17) {
                        int unknown_1;
                        float u, v;
                        if (submeshes[i].unknown_3 == 1) {
                            int unknown_6;
                        }
                    }
                    if (submeshes[i].material_type == 33) {
                        float u, v;
                    }
                } vertices[submeshes[i].vertices_count] <optimize=false>;
    
                struct { 
                    short v1, v2, v3; 
                } indices[submeshes[i].indices_count] <optimize=false>;
                
                if (submeshes[i].material_type == 0 || submeshes[i].material_type == 1 || submeshes[i].material_type == 17) {
                    local int j;
                    local int k;
                    for (j = 0; j < submeshes[i].unknown_block_count; j++) {
                        for (k = 0; k < submeshes[i].unknown_block[j].subblocks_count; k++) {
                            struct {
                                short unknown1;
                                float unknown2;
                                float unknown3;
                                if (submeshes[i].unknown_3 == 1) {
                                    short unknown4;
                                    short unknown5;
                                }
                            } unknown[submeshes[i].unknown_block[j].subblocks[k].unknown_array_length_3] <optimize=false>;
                        }
                    }
                }
            } verts;
        }
        float unknown_2[50];
        char unknown_3;
    }
 
    if (structure_type == 4) {
        int unknown_1_count;
        struct {
            struct {
                local int reading = true;
                while (reading) {
                    struct {
                        byte unknown_1; // read blocks until this value is 0
                        if (unknown_1 == 0) {
                            reading = false;
                            break;
                        }
                        float unknown_2;
                        float unknown_3;
                        float unknown_4;
                        float unknown_5;
                    } unknown_1;
                }
            } unknown_1;

            int index; // probably current block index
            struct {
                int unknown_1;
                struct {
                    int unknown_1;
                    int unknown_2;
                } unknown_2[unknown_1];
                float unknown_3;
                float unknown_4;
                float unknown_5;
            } unknown_3;

            int unknown_4_length;
            char unknown_4[unknown_4_length];
            int unknown_5_length;
            int unknown_5[unknown_5_length];

            struct {
                int unknown_1;
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
                } unknown_2[unknown_1];    
                byte unknown_3[unknown_1];
            } unknown_6;
        } unknown_1[unknown_1_count] <optimize=false>;

        int unknown_2_count;
        int unknown_2[unknown_2_count]; // array of ints that sometimes matches the triangle count

        int unknown_3_count;
        struct {
            int triangles_count_1;
            int unknown_1;
            byte texture_name_length;
            char texture_name[texture_name_length];
            int triangles_count_2;

            byte vertices_type; // 0 - float, 1 - short
            if (vertices_type == 1) {
                float unknown_3; // probably world coordinates
                float unknown_4;
                float unknown_5;
            }
            struct {
                if (vertices_type == 0) {
                    float x;
                    float y;
                    float z;
                } else {
                    short x;
                    short y;
                    short z;
                }
                char unknown_1[4];
                float u;
                float v;
            } vertices[triangles_count_2*3] <optimize=false>;

            struct {
                int unknown_1;
                if (unknown_1 > 0) {
                    struct {
                        int unknown_1;
                        int unknown_2;
                        int unknown_3_length;
                        char unknown_3[unknown_3_length];
                        int unknown_4_length;
                        char unknown_4[unknown_4_length];
                        struct {
                            int unknown_1;
                            struct {
                                short unknown_1;
                                float unknown_2;
                                float unknown_3;
                                float unknown_4;
                                float unknown_5;
                            } unknown_2[unknown_1*3];
                            float unknown_3;
                        } unknown_5;
                    } unknown_2[unknown_1] <optimize=false>;
                    struct {
                        int unknown_1;
                        int unknown_2;
                    } unknown_10[triangles_count_2];
                }
            } unknown_2[unknown_2_count] <optimize=false>;

        } unknown_3[unknown_3_count] <optimize=false>;
    }
 
    if (structure_type == 5) {
        int mesh_count;
        int unknown_1;
        int unknown_2;
        byte unknown_3;
        byte texture_name_length;
        char texture_name[texture_name_length];
        byte unknown_4;
        int vertices_count;
        int indices_count;
        float unknown_5[19];
        byte unknown_6;
        float unknown_7;
        float unknown_8;
        float unknown_9;
        struct {
            short x;
            short y;
            short z;
            char unknown_1[4];
            float u;
            float v;
        } vertices[vertices_count];
        struct {
            short unknown_1;
            short unknown_2;
            short unknown_3;
        } indices[indices_count];
        float unknown_10;
        float unknown_11;
        float unknown_12;
        float unknown_13[12];
    }
} model;
```
