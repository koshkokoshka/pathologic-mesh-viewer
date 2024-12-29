# Pathologic Mesh Viewer

A 3D model viewer for Pathologic Classic HD that supports the .mesh file format used in the game.

<img src="screenshot.png" alt="Pathologic">

This tool makes possible to load and explore the game's 3D assets.

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

### .mesh file structure (NOT COMPLETE)
See `mesh.c` for details.
```
struct {
    unsigned int model_type_or_submesh_count;
    if (model_type_or_submesh_count == 0) {
        unsigned int submesh_count; // read value again if previous was 0
    }
    
    struct {
       unsigned int unknown1;
       unsigned int unknown2;
       if (unknown2 == 1) {
           unsigned char unknown3; // if "unknown2 == 1" than there are 1 byte value
       }
       
       unsigned char texture_name_length;
       char texture_name[texture_name_length];
       unsigned char unknown4; // probably "texture has alpha channel" flag
       
       if (unknown2 == 5) {
           float unknown5[4]; // if "unknown2 == 5" than there are 4 float values
       }

       unsigned int vertices_count;
       unsigned int indices_count;

       float unknown_floats[19]; // there are always 19 float values here, but their purpose is unknown
   
       if (model_type_or_submesh_count == 0) {
          int unknown_blocks_count;
          struct {
             int block_unknown1;
             float block_unknown2[15];
             int block_unknown3;
             if (block_unknown3 == 1) {
                int block_unknown4;
                int block_unknown5;
                int block_unknown6;
                char block_unknown7[block_unknown6];
                int block_unknown8;
                char block_unknown9[block_unknown8];
                int block_unknown10;
                int block_unknown11;
                struct {
                    short subblock_unknown1[4];
                } unknown_subblocks[block_unknown11];
                float block_unknown12;
             }
          } unknown_blocks[unknown_blocks_count];
       }

    } submesh_info[submesh_count];

    unsgined char unknown[13]; // unknown 13 bytes

    struct {
       struct {
           short x;
           short y;
           short z;
           char unknown[4]; // unknown 4 bytes
           float u;
           float v;
       } vertices[vertices_count];

       struct {
           short v1;
           short v2;
           short v3;
       } indices[indices_count];

    } submeshes[submesh_count];
} mesh;
```

