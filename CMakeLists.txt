cmake_minimum_required(VERSION 3.26)
project(pathologic_mesh_viewer C)

set(CMAKE_C_STANDARD 11)

add_executable(pathologic_mesh_viewer WIN32
        src/main.c
        src/clip_line.c
        src/draw_globals.c
        src/draw_line.c
        src/draw_triangle.c
        src/load_mesh.c
        src/load_texture.c
        src/app.rc
)
target_link_libraries(pathologic_mesh_viewer Shlwapi.lib Windowscodecs)