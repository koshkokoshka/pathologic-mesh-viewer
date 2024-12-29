cmake_minimum_required(VERSION 3.26)
project(pathologic_mesh_viewer C)

set(CMAKE_C_STANDARD 11)

add_executable(pathologic_mesh_viewer WIN32 main.c mesh.c texture.c app.rc)
target_link_libraries(pathologic_mesh_viewer Shlwapi.lib Windowscodecs)