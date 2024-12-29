#include <windows.h>
#include <windowsx.h>
#define _USE_MATH_DEFINES
#include <float.h>
#include <math.h>

#include "mesh.h"

// Window menu
#define MENU_ITEM_ID_DRAW_WIREFRAME 1
#define MENU_ITEM_ID_DRAW_POINTS 2
#define MENU_ITEM_ID_DRAW_FACES 3
#define MENU_ITEM_ID_DRAW_TEXTURE 4
#define MENU_ITEM_ID_OPEN 10
#define MENU_ITEM_ID_EXIT 20
#define MENU_ITEM_ID_HELP 30
HMENU g_menu;

// Screen buffer
const int g_screen_width = 800;
const int g_screen_height = 600;
DWORD *g_screen_color;
float *g_screen_depth;

// Rendering variables
float g_screen_dimension;
float g_screen_dimension_offset_x;
float g_screen_dimension_offset_y;

// Camera variables
float g_camera_x = 0.0f;
float g_camera_y = 0.0f;
float g_camera_z = 0.0f;
float g_camera_rot = 0.0f;
float g_camera_yaw = 0.0f;

// Loaded mesh
Mesh g_mesh;
BOOL g_mesh_loaded = FALSE;

static void DrawPoint_Unsafe(int x, int y, float z, DWORD color)
{
    int index = (y * g_screen_width) + x;
    float z_inv = 1.0f / z;
    if (z_inv >= g_screen_depth[index]) {
        g_screen_color[index] = color;
        g_screen_depth[index] = z_inv;
    }
}

static void DrawPoint(int x, int y, float z, DWORD color)
{
    if (x >= 0 && x < g_screen_width && y >= 0 && y < g_screen_height) {
        DrawPoint_Unsafe(x, y, z, color);
    }
}

BOOL ClipLine(int *x1, int *y1, int *x2, int *y2, int x_min, int y_min, int x_max, int y_max)
{
    // Clipping parameters
    float a = 0.0f;
    float b = 1.0f;

    // Calculate line direction
    float dx = (float)(*x2 - *x1);
    float dy = (float)(*y2 - *y1);

    // Compute directions and distances vectors for the four clipping boundaries
    float p[4];
    p[0] = -dx;
    p[1] =  dx;
    p[2] = -dy;
    p[3] =  dy;

    float q[4];
    q[0] = (float)(*x1 - x_min);
    q[1] = (float)(x_max - *x1);
    q[2] = (float)(*y1 - y_min);
    q[3] = (float)(y_max - *y1);

    // Update u1 and u2 based on the clipping boundaries
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0) {
            if (q[i] < 0) {
                return FALSE; // line is parallel and outside the clipping edge
            }
        } else {
            float r = q[i] / p[i]; // ratio of the distances
            if (p[i] < 0) {
                if (r > a) { a = r; }
            } else {
                if (r < b) { b = r; }
            }
        }
    }

    // Check if line is outside the clipping region
    if (a > b) {
        return FALSE; // line is outside
    }

    // Update points based on the clipped values
    if (b < 1.0) {
        *x2 = *x1 + (int)roundf(dx * b);
        *y2 = *y1 + (int)roundf(dy * b);
    }
    if (a > 0.0) {
        *x1 = *x1 + (int)roundf(dx * a);
        *y1 = *y1 + (int)roundf(dy * a);
    }
    return TRUE; // line is within the clipping region
}

void DrawLine_Unsafe(int x1, int y1, int x2, int y2, DWORD color)
{
    // Calculate the absolute difference
    int dx =  abs(x2 - x1);
    int dy = -abs(y2 - y1);

    // In which direction coordinates should move, either up (-1) or down (1)
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;

    int error = dx + dy;

    // Loop until the current point (x1, y1) reaches the endpoint (x2, y2)
    while (1) {

        // Plot pixel
        int i = y1 * g_screen_width + x1;
        g_screen_color[i] = color;
        g_screen_depth[i] = FLT_MAX;

        // Is destination point reached?
        if (x1 == x2 && y1 == y2) {
            break;
        }

        // Depending on the error value, either move horizontally, vertically, or both
        int e2 = error * 2;
        if (e2 >= dy) { error += dy; x1 += sx; }
        if (e2 <= dx) { error += dx; y1 += sy; }
    }
}

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
                  float cx, float cy, float z3, float u3, float v3)
{
    if (texture.width == 0 || texture.height == 0) { return; } // safety check

    const int x1 = (int)ax;
    const int y1 = (int)ay;
    const int x2 = (int)bx;
    const int y2 = (int)by;
    const int x3 = (int)cx;
    const int y3 = (int)cy;

    // Find triangle bounds
    int min_x = (x1 < x2) ? ( (x1 < x3) ? x1 : x3 ) : ( (x2 < x3) ? x2 : x3 );
    int min_y = (y1 < y2) ? ( (y1 < y3) ? y1 : y3 ) : ( (y2 < y3) ? y2 : y3 );
    int max_x = (x1 > x2) ? ( (x1 > x3) ? x1 : x3 ) : ( (x2 > x3) ? x2 : x3 );
    int max_y = (y1 > y2) ? ( (y1 > y3) ? y1 : y3 ) : ( (y2 > y3) ? y2 : y3 );

    // Clip to the screen
    if (min_x < 0) { min_x = 0; }
    if (max_x > g_screen_width) { max_x = g_screen_width; }
    if (min_y < 0) { min_y = 0; }
    if (max_y > g_screen_height) { max_y = g_screen_height; }

    // Set up edge functions
    const int a12 = y2 - y3;
    const int b12 = x3 - x2;
    const int a23 = y3 - y1;
    const int b23 = x1 - x3;
    const int a31 = y1 - y2;
    const int b31 = x2 - x1;

    float w1_row = a12 * (min_x - x3) + b12 * (min_y - y3);
    float w2_row = a23 * (min_x - x1) + b23 * (min_y - y1);
    float w3_row = a31 * (min_x - x2) + b31 * (min_y - y2);

    // Precompute the inverse of the triangle's area
    const float area = ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
    if (area == 0) {
        return; // nothing to draw
    }
    const float area_inv = 1.0f / area;

    const float z1_inv = 1.0f / z1;
    const float z2_inv = 1.0f / z2;
    const float z3_inv = 1.0f / z3;

    for (int y = min_y; y < max_y; y++) {

        // Barycentric coordinates at the start of the row
        float w1 = w1_row;
        float w2 = w2_row;
        float w3 = w3_row;

        for (int x = min_x; x < max_x; x++) {

            // If p is on the right side of all three edges, render pixel
            if (w1 >= 0 && w2 >= 0 && w3 >= 0) {

                const size_t i = (y * g_screen_width) + x;

                // Interpolate triangle coordinates using barycentric coordinates
                const float a = w1 * area_inv;
                const float b = w2 * area_inv;
                const float c = w3 * area_inv;

                // Interpolate depth
                const float z = (a * z1_inv) + (b * z2_inv) + (c * z3_inv);
                if (z > g_screen_depth[i]) { // Test depth

                    // Interpolate texture coordinates
                    float u = (u1 * a * z1_inv + u2 * b * z2_inv + u3 * c * z3_inv) / z;
                    float v = (v1 * a * z1_inv + v2 * b * z2_inv + v3 * c * z3_inv) / z;

                    // Wrap texture coordinates to make the texture repeatable
                    u = u - floorf(u);
                    v = v - floorf(v);

                    int tx = (int)(u * (float)texture.width);
                    int ty = (int)(v * (float)texture.height);
                    int tw = (int)texture.width - 1;
                    int th = (int)texture.height - 1;
                    tx = tx < 0 ? 0 : (tx > tw ? tw : tx);
                    ty = ty < 0 ? 0 : (ty > th ? th : ty);
                    DWORD color = texture.data[(ty * texture.width) + tx];

                    g_screen_color[i] = color;
                    g_screen_depth[i] = z;
                }
            }

            // Move to the next pixel
            w1 += a12;
            w2 += a23;
            w3 += a31;
        }

        // Move to the next scanline
        w1_row += b12;
        w2_row += b23;
        w3_row += b31;
    }
}

void CalculateBounds()
{
    float min_x =  1000.0f, min_y =  1000.0f, min_z =  1000.0f;
    float max_x = -1000.0f, max_y = -1000.0f, max_z = -1000.0f;
    for (int i = 0; i < g_mesh.triangle_count; ++i) {
        MeshTriangle triangle = g_mesh.triangles[i];
        MeshPoint a = g_mesh.points[triangle.a];
        MeshPoint b = g_mesh.points[triangle.b];
        MeshPoint c = g_mesh.points[triangle.c];
        min_x = min(min_x, a.x);
        min_x = min(min_x, b.x);
        min_x = min(min_x, c.x);
        min_y = min(min_y, a.y);
        min_y = min(min_y, b.y);
        min_y = min(min_y, c.y);
        min_z = min(min_y, a.z);
        min_z = min(min_y, b.z);
        min_z = min(min_y, c.z);
        max_x = max(max_x, a.x);
        max_x = max(max_x, b.x);
        max_x = max(max_x, c.x);
        max_y = max(max_y, a.y);
        max_y = max(max_y, b.y);
        max_y = max(max_y, c.y);
        max_z = max(max_y, a.z);
        max_z = max(max_y, b.z);
        max_z = max(max_y, c.z);
    }

    float bounds = fabsf(max_x);
    bounds = max(bounds, fabsf(min_x));
    bounds = max(bounds, fabsf(max_y));
    bounds = max(bounds, fabsf(min_y));
    bounds = max(bounds, fabsf(max_z));
    bounds = max(bounds, fabsf(min_z));

    g_camera_x = 0;
    g_camera_y = 0;
    g_camera_z = bounds * 3;
    g_camera_rot = 0;
    g_camera_yaw = 0;
}

void TryLoadMesh(HWND window, const char *path)
{
    g_mesh_loaded = Mesh_LoadFromPathologicFormat(&g_mesh, path);
    if (!g_mesh_loaded) {
        MessageBox(
            window,
            TEXT("An error occurred while trying to load the selected model. Please note that Pathologic Mesh Viewer is still under development and may fail to load some game meshes."),
            TEXT("Failed to load .mesh file"),
            MB_OK | MB_ICONWARNING
        );
        return;
    }
    CalculateBounds();
}

static void Point_Rotate(float *x, float *y, float sin, float cos)
{
    float rx = (*x) * cos - (*y) * sin;
    float ry = (*x) * sin + (*y) * cos;
    *x = rx;
    *y = ry;
}

void Window_DrawViewport(BOOL draw_wireframe, BOOL draw_points, BOOL draw_faces, BOOL draw_texture)
{
    // Precompute screen-related values
    g_screen_dimension = min(g_screen_width, g_screen_height);
    g_screen_dimension_offset_x = (g_screen_width  - g_screen_dimension) / 2;
    g_screen_dimension_offset_y = (g_screen_height - g_screen_dimension) / 2;

    // Clear buffers
    for (int i = 0; i < g_screen_width * g_screen_height; ++i) {
        g_screen_color[i] = 0x393029;
    }
    for (int i = 0; i < g_screen_width * g_screen_height; ++i) {
        g_screen_depth[i] = 0.0f;
    }

    if (!g_mesh_loaded) {
        return; // nothing to draw
    }

    float mesh_rot_cos = cosf(g_camera_rot);
    float mesh_rot_sin = sinf(g_camera_rot);
    float mesh_yaw_cos = cosf(g_camera_yaw);
    float mesh_yaw_sin = sinf(g_camera_yaw);

    if (draw_texture) {
        if (g_mesh.texture_count > 0) {
            Texture texture = g_mesh.textures[0];

            for (int y = 0; y < texture.height; ++y) {
                int sy = y/2;
                if (sy < 0 || sy >= g_screen_height) { continue; }
                for (int x = 0; x < texture.width; ++x) {
                    int sx = x/2;
                    if (sx < 0 || sx >= g_screen_width) { continue; }
                    g_screen_color[sy * g_screen_width + sx] = texture.data[y * texture.width + x];
                }
            }
        }
    }

    // Render mesh wireframe
    for (int i = 0; i < g_mesh.triangle_count; ++i) {
        MeshTriangle t = g_mesh.triangles[i];
        MeshPoint a = g_mesh.points[t.a];
        MeshPoint b = g_mesh.points[t.b];
        MeshPoint c = g_mesh.points[t.c];

        float ax = a.x;
        float ay = a.y;
        float az = a.z;
        ay *= -1;
        float bx = b.x;
        float by = b.y;
        float bz = b.z;
        by *= -1;
        float cx = c.x;
        float cy = c.y;
        float cz = c.z;
        cy *= -1;

        // Model-space rotation
        Point_Rotate(&ax, &az, mesh_rot_sin, mesh_rot_cos);
        Point_Rotate(&bx, &bz, mesh_rot_sin, mesh_rot_cos);
        Point_Rotate(&cx, &cz, mesh_rot_sin, mesh_rot_cos);

        Point_Rotate(&ay, &az, mesh_yaw_sin, mesh_yaw_cos);
        Point_Rotate(&by, &bz, mesh_yaw_sin, mesh_yaw_cos);
        Point_Rotate(&cy, &cz, mesh_yaw_sin, mesh_yaw_cos);

        // Model-space transform
        ax += g_camera_x;
        ay += g_camera_y;
        az += g_camera_z;
        bx += g_camera_x;
        by += g_camera_y;
        bz += g_camera_z;
        cx += g_camera_x;
        cy += g_camera_y;
        cz += g_camera_z;

        // Perspective divide
        ax /= az;
        ay /= az;
        bx /= bz;
        by /= bz;
        cx /= cz;
        cy /= cz;

        // Backface culling
        if (az <= 0) { continue; }
        if (bz <= 0) { continue; }
        if (cz <= 0) { continue; }

        // Screen-space transform (remap from [-0.5, 0.5] to [0, screen_dimension]
        ax = (0.5f + ax) * g_screen_dimension + g_screen_dimension_offset_x;
        ay = (0.5f + ay) * g_screen_dimension + g_screen_dimension_offset_y;
        bx = (0.5f + bx) * g_screen_dimension + g_screen_dimension_offset_x;
        by = (0.5f + by) * g_screen_dimension + g_screen_dimension_offset_y;
        cx = (0.5f + cx) * g_screen_dimension + g_screen_dimension_offset_x;
        cy = (0.5f + cy) * g_screen_dimension + g_screen_dimension_offset_y;

        if (draw_faces) {
            DrawTriangle(g_mesh.textures[0],
                         ax, ay, az, a.u, a.v,
                         bx, by, bz, b.u, b.v,
                         cx, cy, cz, c.u, c.v);
        }
        if (draw_wireframe) {
            DrawLine(ax, ay, bx, by, 0x66FF66);
            DrawLine(bx, by, cx, cy, 0x66FF66);
            DrawLine(cx, cy, ax, ay, 0x66FF66);
        }
    }

    // Render mesh points
    if (draw_points) {
        for (int i = 0; i < g_mesh.point_count; ++i) {
            MeshPoint *point = &g_mesh.points[i];

            float x = point->x;
            float y = point->y;
            float z = point->z;
            y *= -1;

            // Model-space rotation
            Point_Rotate(&x, &z, mesh_rot_sin, mesh_rot_cos);
            Point_Rotate(&y, &z, mesh_yaw_sin, mesh_yaw_cos);

            // Model-space transform
            x += g_camera_x;
            y += g_camera_y;
            z += g_camera_z;

            // Culling
            if (z <= 0) {
                continue;
            }

            // Perspective divide
            x /= z;
            y /= z;

            // Screen-space transform (remap from [-0.5, 0.5] to [0, screen_dimension]
            x = (0.5f + x) * g_screen_dimension + g_screen_dimension_offset_x;
            y = (0.5f + y) * g_screen_dimension + g_screen_dimension_offset_y;

            // Draw points
            DrawPoint(x, y, z, 0xFF0000);
        }
    }
}

BOOL FileSelectDialog_Show(OUT char *result, int result_max, HWND parent, LPCSTR filter)
{
    OPENFILENAME dialog_params;
    ZeroMemory(&dialog_params, sizeof(OPENFILENAME));
    dialog_params.lStructSize = sizeof(OPENFILENAME);
    dialog_params.hwndOwner = parent;
    dialog_params.lpstrFile = result;
    dialog_params.nMaxFile = result_max;
    dialog_params.lpstrFilter = filter;
    dialog_params.Flags = OFN_PATHMUSTEXIST;
    return GetOpenFileName(&dialog_params);
}

static BOOL WindowMenu_IsItemChecked(HMENU menu, UINT menu_item_id)
{
    return (GetMenuState(menu, menu_item_id, MF_BYCOMMAND) & MF_CHECKED) == MF_CHECKED;
}

static void WindowMenu_ToggleItem(HMENU menu, UINT menu_item_id)
{
    BOOL checked = WindowMenu_IsItemChecked(menu, menu_item_id);
    CheckMenuItem(menu, menu_item_id, MF_BYCOMMAND | (!checked ? MF_CHECKED : MF_UNCHECKED));
}

BOOL Window_Create(HWND window)
{
    // Initialize buffers
    g_screen_color = HeapAlloc(GetProcessHeap(), 0, g_screen_width * g_screen_height * sizeof(DWORD));
    if (!g_screen_color) {
        return FALSE;
    }
    g_screen_depth = HeapAlloc(GetProcessHeap(), 0, g_screen_width * g_screen_height * sizeof(float));
    if (!g_screen_depth) {
        return FALSE;
    }

    // Create menu
    g_menu = CreateMenu();
    HMENU menu_file = CreatePopupMenu();
    AppendMenu(menu_file, MF_STRING, MENU_ITEM_ID_OPEN, "&Open...");
    AppendMenu(menu_file, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu_file, MF_STRING, MENU_ITEM_ID_EXIT, TEXT("E&xit"));
    AppendMenu(g_menu, MF_POPUP, (UINT_PTR)menu_file, "File");
    HMENU menu_view = CreatePopupMenu();
    AppendMenu(menu_view, MF_STRING, MENU_ITEM_ID_DRAW_WIREFRAME, "Draw wireframe\tF2");
    AppendMenu(menu_view, MF_STRING | MF_CHECKED, MENU_ITEM_ID_DRAW_POINTS, "Draw points\tF3");
    AppendMenu(menu_view, MF_STRING | MF_CHECKED, MENU_ITEM_ID_DRAW_FACES, "Draw faces\tF4");
    AppendMenu(menu_view, MF_STRING, MENU_ITEM_ID_DRAW_TEXTURE, "Draw texture preview\tF5");
    AppendMenu(g_menu, MF_POPUP, (UINT_PTR)menu_view, "View");
    HMENU menu_help = CreatePopupMenu();
    AppendMenu(menu_help, MF_STRING, MENU_ITEM_ID_HELP, "Help\tF1");
    AppendMenu(g_menu, MF_POPUP, (UINT_PTR)menu_help, "View");
    SetMenu(window, g_menu);

    // Allow to drag and drop files to window
    DragAcceptFiles(window, TRUE);

    return TRUE;
}

void ShowHelp(HWND window)
{
    MessageBox(window, TEXT("For the latest updates, visit: https://github.com/koshkokoshka/pathologic-mesh-viewer"), TEXT("Help"), MB_OK | MB_ICONINFORMATION);
}

void Window_OnKeyDown(HWND window, WPARAM w_param)
{
    switch (w_param) {
        case VK_F1: ShowHelp(window); break;
        case VK_F2: WindowMenu_ToggleItem(g_menu, MENU_ITEM_ID_DRAW_WIREFRAME); break;
        case VK_F3: WindowMenu_ToggleItem(g_menu, MENU_ITEM_ID_DRAW_POINTS); break;
        case VK_F4: WindowMenu_ToggleItem(g_menu, MENU_ITEM_ID_DRAW_FACES); break;
        case VK_F5: WindowMenu_ToggleItem(g_menu, MENU_ITEM_ID_DRAW_TEXTURE); break;
        default: return; // do not redraw window
    }
    RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
}

void Window_OnCommand(HWND window, WPARAM w_param)
{
    switch (w_param) {
        case MENU_ITEM_ID_DRAW_WIREFRAME:
        case MENU_ITEM_ID_DRAW_POINTS:
        case MENU_ITEM_ID_DRAW_FACES:
        case MENU_ITEM_ID_DRAW_TEXTURE:
            WindowMenu_ToggleItem(g_menu, w_param);
            RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
            break;
        case MENU_ITEM_ID_OPEN:
            TCHAR path[MAX_PATH];
            path[0] = '\0';
            if (FileSelectDialog_Show(path, sizeof(path), window, "Pathologic 3D mesh data (.mesh)\0*.mesh\0\0")) {
                TryLoadMesh(window, path);
            }
            RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
            break;
        case MENU_ITEM_ID_EXIT:
            PostQuitMessage(0);
            break;
        case MENU_ITEM_ID_HELP:
            ShowHelp(window);
            break;
    }
}

void Window_OnMouseWheel(HWND window, WPARAM w_param)
{
    float delta = (float)GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA;
    g_camera_z = g_camera_z * powf(0.95f, delta);
    RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
}

void Window_OnLeftMouseDown(HWND window)
{
    SetCapture(window);
}

void Window_OnLeftMouseUp(HWND window)
{
    ReleaseCapture();
}

void Window_OnMouseMove(HWND window, LPARAM l_param)
{
    static int g_mouse_x;
    static int g_mouse_y;

    int x = GET_X_LPARAM(l_param);
    int y = GET_Y_LPARAM(l_param);
    if (GetCapture() == window) {
        g_camera_rot += (float)(x - g_mouse_x) * 0.01f;
        g_camera_yaw += (float)(y - g_mouse_y) * 0.01f;
        RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
    }
    g_mouse_x = x;
    g_mouse_y = y;
}

void Window_OnDropFiles(HWND window, WPARAM w_param)
{
    HDROP drop = (HDROP)w_param;
    TCHAR path[MAX_PATH];
    if (DragQueryFile(drop, 0, path, MAX_PATH) != 0) {
        TryLoadMesh(window, path);
    }
    DragFinish(drop);
    RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
}

void Window_OnPaint(HWND window)
{
    PAINTSTRUCT ps;
    BeginPaint(window, &ps);

    BOOL draw_wireframe = WindowMenu_IsItemChecked(g_menu, MENU_ITEM_ID_DRAW_WIREFRAME);
    BOOL draw_points = WindowMenu_IsItemChecked(g_menu, MENU_ITEM_ID_DRAW_POINTS);
    BOOL draw_faces = WindowMenu_IsItemChecked(g_menu, MENU_ITEM_ID_DRAW_FACES);
    BOOL draw_texture = WindowMenu_IsItemChecked(g_menu, MENU_ITEM_ID_DRAW_TEXTURE);
    Window_DrawViewport(draw_wireframe, draw_points, draw_faces, draw_texture);

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_screen_width;
    bmi.bmiHeader.biHeight = -g_screen_height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(ps.hdc,
        ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom,
        ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom,
        g_screen_color, &bmi, DIB_RGB_COLORS, SRCCOPY);

    if (!g_mesh_loaded) {
        SelectObject(ps.hdc, GetStockObject(DEFAULT_GUI_FONT));
        RECT client_rect;
        GetClientRect(window, &client_rect);
        DrawText(ps.hdc, TEXT("Select Pathologic .mesh with File -> Open..."), -1, &client_rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }

    EndPaint(window, &ps);
}

LRESULT CALLBACK Window_Process(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message) {
        case WM_CREATE:
            if (!Window_Create(window)) {
                PostQuitMessage(1);
                return 1;
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_COMMAND:
            Window_OnCommand(window, w_param);
            return 0;

        case WM_KEYDOWN:
            Window_OnKeyDown(window, w_param);
            return 0;

        case WM_MOUSEWHEEL:
            Window_OnMouseWheel(window, w_param);
            return 0;

        case WM_LBUTTONDOWN:
            Window_OnLeftMouseDown(window);
            return 0;

        case WM_LBUTTONUP:
            Window_OnLeftMouseUp(window);
            return 0;

        case WM_MOUSEMOVE:
            Window_OnMouseMove(window, l_param);
            return 0;

        case WM_DROPFILES:
            Window_OnDropFiles(window, w_param);
            return 0;

        case WM_PAINT:
            Window_OnPaint(window);
            return 0;

        default:
            return DefWindowProc(window, message, w_param, l_param);
    }
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    // Initialize COM library (required for WIC image loading)
    if (CoInitialize(NULL) != S_OK) {
        return 1;
    }

    // Initialize WIC library
    if (CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC, &IID_IWICImagingFactory, (LPVOID*)&g_wic_factory) != S_OK) {
        return 1;
    }

    // Register window class
    WNDCLASS window_class;
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.style = CS_OWNDC;
    window_class.lpfnWndProc = Window_Process;
    window_class.hInstance = hInstance;
    window_class.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName = "PATHOLOGIC_MODEL_VIEWER";
    if (!RegisterClass(&window_class)) {
        return 1;
    }

    // Define window styles
    DWORD window_style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX;

    RECT window_rect = { 0, 0, g_screen_width, g_screen_height };
    if (AdjustWindowRect(&window_rect, window_style, FALSE)) {
        window_rect.right -= window_rect.left;
        window_rect.bottom -= window_rect.top;
        window_rect.left = (GetSystemMetrics(SM_CXSCREEN) - window_rect.right) / 2;
        window_rect.top = (GetSystemMetrics(SM_CYSCREEN) - window_rect.bottom) / 2;
    }

    // Create window
    HWND window = CreateWindowEx(
        WS_EX_COMPOSITED, window_class.lpszClassName, TEXT("Pathologic Mesh Viewer v0.1"), window_style,
        window_rect.left, window_rect.top, window_rect.right, window_rect.bottom,
        NULL, NULL, hInstance, NULL);
    if (window == INVALID_HANDLE_VALUE) {
        return 1;
    }
    ShowWindow(window, nShowCmd);

    // Start main loop
    MSG message;
    while (GetMessage(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}
