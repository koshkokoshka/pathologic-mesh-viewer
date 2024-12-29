#include "draw.h"

// Screen buffer
int g_screen_width = 800;
int g_screen_height = 600;
DWORD *g_screen_color;
float *g_screen_depth;

// Precomputed rendering variables
float g_screen_dimension;
float g_screen_dimension_offset_x;
float g_screen_dimension_offset_y;

// Camera-related variables
float g_camera_x = 0.0f;
float g_camera_y = 0.0f;
float g_camera_z = 0.0f;
float g_camera_rot = 0.0f;
float g_camera_yaw = 0.0f;