#pragma once

#include <windows.h>

#include <float.h>

#include "texture.h"
#include "clip_line.h"

//
// Global draw variables
//

// Screen buffer
extern int g_screen_width;
extern int g_screen_height;
extern DWORD *g_screen_color;
extern float *g_screen_depth;

// Precomputed rendering variables
extern float g_screen_dimension;
extern float g_screen_dimension_offset_x;
extern float g_screen_dimension_offset_y;

static void PrecomputeRenderingVariables()
{
    g_screen_dimension = min(g_screen_width, g_screen_height);
    g_screen_dimension_offset_x = (g_screen_width  - g_screen_dimension) / 2;
    g_screen_dimension_offset_y = (g_screen_height - g_screen_dimension) / 2;
}

// Camera-related variables
extern float g_camera_x;
extern float g_camera_y;
extern float g_camera_z;
extern float g_camera_rot;
extern float g_camera_yaw;

//
// Draw functions
//

static void DrawPoint_Unsafe(int x, int y, DWORD color)
{
    size_t index = (y * g_screen_width) + x;
    g_screen_color[index] = color;
    g_screen_depth[index] = FLT_MAX;
}

static void DrawPoint(int x, int y, DWORD color)
{
    if (x >= 0 && x < g_screen_width && y >= 0 && y < g_screen_height) {
        DrawPoint_Unsafe(x, y, color);
    }
}

void DrawLine_Unsafe(int x1, int y1, int x2, int y2, DWORD color);

static void DrawLine(int x1, int y1, int x2, int y2, DWORD color)
{
    // Clip line inside screen bounds
    if (ClipLine(&x1, &y1, &x2, &y2, 0, 0, g_screen_width-1, g_screen_height-1) == FALSE) {
        return; // line is outside, nothing to draw
    }

    // Draw clipped line
    DrawLine_Unsafe(x1, y1, x2, y2, color);
}

void DrawTriangle(Texture texture,
                  float ax, float ay, float z1, float u1, float v1,
                  float bx, float by, float z2, float u2, float v2,
                  float cx, float cy, float z3, float u3, float v3);
