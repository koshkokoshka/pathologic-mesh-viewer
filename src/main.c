#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <shlwapi.h>
#include <stdio.h>

#include "math.h"
#include "draw.h"
#include "mesh.h"
#include "clip_triangle.h"

#define APP_NAME "Pathologic Mesh Viewer v0.5"

// Window menu
#define MENU_ITEM_ID_DRAW_WIREFRAME 1
#define MENU_ITEM_ID_DRAW_POINTS 2
#define MENU_ITEM_ID_DRAW_FACES 3
#define MENU_ITEM_ID_DRAW_TRANSPARENCY 4
#define MENU_ITEM_ID_OPEN 10
#define MENU_ITEM_ID_EXPORT_OBJ 11
#define MENU_ITEM_ID_EXIT 20
#define MENU_ITEM_ID_HELP 30
HMENU g_menu;

// Window statusbar
#define IDC_STATUSBAR 10
HWND g_statusbar;

// Loaded mesh
Mesh g_mesh;
BOOL g_mesh_loaded = FALSE;
BOOL g_mesh_error = FALSE;
char g_mesh_path[MAX_PATH] = "\0";

char g_mesh_directory[MAX_PATH] = "\0";
int g_mesh_directory_files_count = 0;
int g_mesh_directory_file_number = 0;
struct { char name[MAX_PATH]; } *g_mesh_directory_files;

void Viewport_CalculateBounds()
{
    float min_x =  1000.0f, min_y =  1000.0f, min_z =  1000.0f;
    float max_x = -1000.0f, max_y = -1000.0f, max_z = -1000.0f;
    for (int i1 = 0; i1 < g_mesh.submesh_count; ++i1) {
        MeshSubmesh submesh = g_mesh.submeshes[i1];
        for (int i2 = 0; i2 < submesh.triangle_count; ++i2) {
            MeshTriangle triangle = submesh.triangles[i2];
            MeshPoint a = submesh.points[triangle.a];
            MeshPoint b = submesh.points[triangle.b];
            MeshPoint c = submesh.points[triangle.c];
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

BOOL FormatNumberWithCommas(char *output, int output_size, int number)
{
    // Get the current locale
    LCID locale = GetUserDefaultLCID();

    // Define number formatting options
    NUMBERFMT format;
    ZeroMemory(&format, sizeof(NUMBERFMT));
    format.NumDigits = 0;          // No decimal places
    format.LeadingZero = 0;        // No leading zero for fractional numbers
    format.Grouping = 3;           // Group digits into thousands
    format.lpDecimalSep = ".";     // Decimal separator
    format.lpThousandSep = ",";    // Thousands separator
    format.NegativeOrder = 1;      // Negative numbers format (-1)

    // Format the number
    char buffer[12]; // 12 characters should be enough to store integer as string with null terminator ("-2147483648\0")
    snprintf(buffer, sizeof(buffer), "%d", number);
    return GetNumberFormat(locale, 0, buffer, &format, output, output_size);
}

void TryLoadMesh(HWND window, const char *path)
{
    strcpy(g_mesh_path, path);

    char status[400];
    snprintf(status, sizeof(status), "Loading: %s", g_mesh_path);;
    SendMessage(g_statusbar, SB_SETTEXT, MAKEWPARAM(0, 0), (LPARAM)&status);

    // Get directory name
    char directory[MAX_PATH];
    strcpy(directory, g_mesh_path); // copy full .mesh file path
    if (PathRemoveFileSpec(directory)) {

        if (strcmp(directory, g_mesh_directory) != 0) { // if directory changed
            // List .mesh files in directory
            // and cache file names for navigation
            g_mesh_directory_file_number = 0;
            g_mesh_directory_files_count = 0;
            char find_path[MAX_PATH];
            snprintf(find_path, sizeof(find_path), "%s\\*.mesh", directory);
            WIN32_FIND_DATA find_data;
            HANDLE find = FindFirstFile(find_path, &find_data);
            int directory_files_capacity = 6450; // 6450 - exact number of .mesh files in Pathologic Classic HD
            g_mesh_directory_files = HeapAlloc(GetProcessHeap(), 0, sizeof(g_mesh_directory_files[0]) * directory_files_capacity);
            do {
                if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                    continue;
                }
                if (g_mesh_directory_files_count >= directory_files_capacity) {
                    directory_files_capacity += 1024; // grow capacity
                    g_mesh_directory_files = HeapReAlloc(GetProcessHeap(), 0, g_mesh_directory_files, sizeof(g_mesh_directory_files[0]) * directory_files_capacity);
                }
                strcpy(g_mesh_directory_files[g_mesh_directory_files_count].name, find_data.cFileName);
                g_mesh_directory_files_count++;
            } while (FindNextFile(find, &find_data));
            FindClose(find);
            strcpy(g_mesh_directory, directory); // store current directory path
        }

        // Find current file number
        for (int i = 0; i < g_mesh_directory_files_count; ++i) {
            if (strcmp(g_mesh_directory_files[i].name, PathFindFileName(g_mesh_path)) == 0) {
                g_mesh_directory_file_number = i;
                break;
            }
        }
    }

    // Set window title
    char title[260];
    snprintf(title, sizeof(title), "[%d/%d] %s - "APP_NAME, g_mesh_directory_file_number+1, g_mesh_directory_files_count, PathFindFileName(g_mesh_path));
    SetWindowText(window, title);

    if (g_mesh_loaded) {
        // release previously allocated memory
        for (int i = 0; i < g_mesh.submesh_count; ++i) {
            HeapFree(GetProcessHeap(), 0, g_mesh.submeshes[i].points);
            HeapFree(GetProcessHeap(), 0, g_mesh.submeshes[i].triangles);
            if (g_mesh.submeshes[i].texture.data != fallback_texture.data) { // do not release memory of fallback texture
                HeapFree(GetProcessHeap(), 0, g_mesh.submeshes[i].texture.data);
            }
        }
        HeapFree(GetProcessHeap(), 0, g_mesh.submeshes);
    }
    g_mesh_loaded = Mesh_LoadFromPathologicFormat(&g_mesh, path);
    if (!g_mesh_loaded) {
        g_mesh_error = TRUE;
        SendMessage(g_statusbar, SB_SETTEXT, MAKEWPARAM(0, 0), (LPARAM)&TEXT("Mesh not loaded"));
        return;
    }
    g_mesh_error = FALSE;

    int total_vertices = 0;
    int total_triangles = 0;
    for (int i = 0; i < g_mesh.submesh_count; ++i) {
        total_vertices += g_mesh.submeshes[i].point_count;
        total_triangles += g_mesh.submeshes[i].triangle_count;
    }
    char vertices_string[20];
    FormatNumberWithCommas(vertices_string, sizeof(vertices_string), total_vertices);
    char triangles_string[20];
    FormatNumberWithCommas(triangles_string, sizeof(triangles_string), total_triangles);
    snprintf(status, sizeof(status), "Vertices: %s | Polygons: %s | Materials: %d", vertices_string, triangles_string, g_mesh.submesh_count);
    SendMessage(g_statusbar, SB_SETTEXT, MAKEWPARAM(0, 0), (LPARAM)&status);

    Viewport_CalculateBounds();
}

BOOL ExportLoadedMesh(HWND window, const char *path)
{
    if (!g_mesh_loaded) {
        return FALSE;
    }

    HANDLE obj_file = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (obj_file == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    TCHAR mtl_path[MAX_PATH];
    strcpy(mtl_path, path);
    PathRemoveExtension(mtl_path);
    PathAddExtension(mtl_path, ".mtl");
    HANDLE mtl_file = CreateFile(mtl_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // Write .obj file
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "mtllib %s\n", PathFindFileName(mtl_path));
    WriteFile(obj_file, buffer, strlen(buffer), NULL, NULL);
    snprintf(buffer, sizeof(buffer), "o %s\n", PathFindFileName(g_mesh_path)); // write object name
    WriteFile(obj_file, buffer, strlen(buffer), NULL, NULL);
    int points_offset = 0;
    for (int i = 0; i < g_mesh.submesh_count; ++i) {
        MeshSubmesh submesh = g_mesh.submeshes[i];
        snprintf(buffer, sizeof(buffer), "g %d\n", i+1);
        WriteFile(obj_file, buffer, strlen(buffer), NULL, NULL);
        for (int j = 0; j < submesh.point_count; ++j) {
            MeshPoint point = submesh.points[j];
            snprintf(buffer, sizeof(buffer), "v %f %f %f\n", point.x, point.y, point.z);
            WriteFile(obj_file, buffer, strlen(buffer), NULL, NULL);
        }
        for (int j = 0; j < submesh.point_count; ++j) {
            MeshPoint point = submesh.points[j];
            snprintf(buffer, sizeof(buffer), "vt %f %f\n", point.u, 1.0f-point.v);
            WriteFile(obj_file, buffer, strlen(buffer), NULL, NULL);
        }
        snprintf(buffer, sizeof(buffer), "s 1\n", i); // enable smooth shading
        WriteFile(obj_file, buffer, strlen(buffer), NULL, NULL);
        snprintf(buffer, sizeof(buffer), "usemtl %s\n", PathFindFileName(submesh.texture_path));
        WriteFile(obj_file, buffer, strlen(buffer), NULL, NULL);
        for (int j = 0; j < submesh.triangle_count; ++j) {
            MeshTriangle triangle = submesh.triangles[j];
            snprintf(buffer, sizeof(buffer), "f %d/%d %d/%d %d/%d\n",
                points_offset + triangle.a+1, points_offset + triangle.a+1,
                points_offset + triangle.b+1, points_offset + triangle.b+1,
                points_offset + triangle.c+1, points_offset + triangle.c+1);
            WriteFile(obj_file, buffer, strlen(buffer), NULL, NULL);
        }
        points_offset += submesh.point_count;
    }
    // Write .mtl file
    for (int i = 0; i < g_mesh.submesh_count; ++i) {
        MeshSubmesh submesh = g_mesh.submeshes[i];
        snprintf(buffer, sizeof(buffer), "newmtl %s\n", PathFindFileName(submesh.texture_path));
        WriteFile(mtl_file, buffer, strlen(buffer), NULL, NULL);
        if (submesh.texture.data != fallback_texture.data) { // do not write texture for fallback material
            snprintf(buffer, sizeof(buffer), "map_Kd %s\n", submesh.texture_path);
            WriteFile(mtl_file, buffer, strlen(buffer), NULL, NULL);
        }
    }

    CloseHandle(obj_file);
    CloseHandle(mtl_file);

    return TRUE;
}

BOOL GenerateCheckerboardTexture(Texture *output, int width, int height, int pattern_size, DWORD color1, DWORD color2)
{
    // Initialize texture
    output->width = width;
    output->height = height;
    output->data = HeapAlloc(GetProcessHeap(), 0, height * width * sizeof(DWORD));
    if (output->data == NULL) {
        return FALSE;
    }

    // Draw the checkerboard pattern
    for (UINT y = 0; y < output->height; ++y) {
        for (UINT x = 0; x < output->width; ++x) {
            UINT i = (y * output->width) + x;
            output->data[i] = ((x / pattern_size) + (y / pattern_size)) % 2 == 0 ? color1 : color2;
        }
    }
    return TRUE;
}

void Viewport_Draw(BOOL draw_wireframe, BOOL draw_points, BOOL draw_faces, BOOL draw_transparency)
{
    // Precompute screen-related values
    PrecomputeRenderingVariables();

    // Clear buffers
    for (int i = 0; i < g_screen_width * g_screen_height; ++i) { g_screen_color[i] = 0x635242; } // background color
    for (int i = 0; i < g_screen_width * g_screen_height; ++i) { g_screen_depth[i] = 0.0f; }

    if (!g_mesh_loaded) {
        return; // nothing to draw
    }

    float mesh_rot_cos = cosf(g_camera_rot);
    float mesh_rot_sin = sinf(g_camera_rot);
    float mesh_yaw_cos = cosf(g_camera_yaw);
    float mesh_yaw_sin = sinf(g_camera_yaw);

    // Render mesh wireframe
    for (int i1 = 0; i1 < g_mesh.submesh_count; ++i1) {
        MeshSubmesh submesh = g_mesh.submeshes[i1];
        for (int i2 = 0; i2 < submesh.triangle_count; ++i2) {
            MeshTriangle t = submesh.triangles[i2];
            MeshPoint a = submesh.points[t.a];
            MeshPoint b = submesh.points[t.b];
            MeshPoint c = submesh.points[t.c];

            // Fix coordinates system
            a.y *= -1;
            b.y *= -1;
            c.y *= -1;

            // Model-space rotation
            Point_Rotate(&a.x, &a.z, mesh_rot_sin, mesh_rot_cos);
            Point_Rotate(&b.x, &b.z, mesh_rot_sin, mesh_rot_cos);
            Point_Rotate(&c.x, &c.z, mesh_rot_sin, mesh_rot_cos);

            Point_Rotate(&a.y, &a.z, mesh_yaw_sin, mesh_yaw_cos);
            Point_Rotate(&b.y, &b.z, mesh_yaw_sin, mesh_yaw_cos);
            Point_Rotate(&c.y, &c.z, mesh_yaw_sin, mesh_yaw_cos);

            // Model-space transform
            a.x += g_camera_x;
            a.y += g_camera_y;
            a.z += g_camera_z;
            b.x += g_camera_x;
            b.y += g_camera_y;
            b.z += g_camera_z;
            c.x += g_camera_x;
            c.y += g_camera_y;
            c.z += g_camera_z;

            //
            // Clipping
            //
            // TODO: Clipping is disabled until I resolve mesh scale issues
            /*
            MeshPoint splits[CLIP_MAX_POINTS] = { a, b, c };
            int splits_count = ClipTriangle(splits, NEAR_CLIP);
            for (int i2 = 0; i2 < splits_count; ++i2) {
                a = splits[(i2 * 3) + 0];
                b = splits[(i2 * 3) + 1];
                c = splits[(i2 * 3) + 2];
            */

                // Perspective divide
                a.x /= a.z;
                a.y /= a.z;
                b.x /= b.z;
                b.y /= b.z;
                c.x /= c.z;
                c.y /= c.z;

                // Backface culling
                if (a.z <= 0) { continue; }
                if (b.z <= 0) { continue; }
                if (c.z <= 0) { continue; }

                // Screen-space transform (remap from [-0.5, 0.5] to [0, screen_dimension]
                a.x = (0.5f + a.x) * g_screen_dimension + g_screen_dimension_offset_x;
                a.y = (0.5f + a.y) * g_screen_dimension + g_screen_dimension_offset_y;
                b.x = (0.5f + b.x) * g_screen_dimension + g_screen_dimension_offset_x;
                b.y = (0.5f + b.y) * g_screen_dimension + g_screen_dimension_offset_y;
                c.x = (0.5f + c.x) * g_screen_dimension + g_screen_dimension_offset_x;
                c.y = (0.5f + c.y) * g_screen_dimension + g_screen_dimension_offset_y;

                if (draw_faces) {
                    DrawTriangle(submesh.texture, draw_transparency,
                                 a.x, a.y, a.z, a.u, a.v,
                                 b.x, b.y, b.z, b.u, b.v,
                                 c.x, c.y, c.z, c.u, c.v);
                }
                if (draw_wireframe) {
                    DrawLine(a.x, a.y, b.x, b.y, 0x999922);
                    DrawLine(b.x, b.y, c.x, c.y, 0x999922);
                    DrawLine(c.x, c.y, a.x, a.y, 0x999922);
                }
            /*
            }
            */
        }
    }

    // Render mesh points
    if (draw_points) {
        for (int i1 = 0; i1 < g_mesh.submesh_count; ++i1) {
            MeshSubmesh submesh = g_mesh.submeshes[i1];
            for (int i2 = 0; i2 < submesh.point_count; ++i2) {
                MeshPoint point = submesh.points[i2];

                float x = point.x;
                float y = point.y;
                float z = point.z;
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
                DrawPoint(x, y, 0x66FF66);
            }
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

BOOL FileOpenDialog_Show(OUT char *result, int result_max, HWND parent, LPCSTR filter)
{
    OPENFILENAME dialog_params;
    ZeroMemory(&dialog_params, sizeof(OPENFILENAME));
    dialog_params.lStructSize = sizeof(OPENFILENAME);
    dialog_params.hwndOwner = parent;
    dialog_params.lpstrFile = result;
    dialog_params.nMaxFile = result_max;
    dialog_params.lpstrFilter = filter;
    dialog_params.Flags = OFN_PATHMUSTEXIST;
    return GetSaveFileName(&dialog_params);
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
    // Initialize fallback texture
    GenerateCheckerboardTexture(&fallback_texture, 64, 64, 8, 0xFF000000, 0xFFFF00FF);

    // Create menu
    g_menu = CreateMenu();
    HMENU menu_file = CreatePopupMenu();
    AppendMenu(menu_file, MF_STRING, MENU_ITEM_ID_OPEN, "&Open...");
    AppendMenu(menu_file, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu_file, MF_STRING, MENU_ITEM_ID_EXPORT_OBJ, "&Export as .OBJ...");
    AppendMenu(menu_file, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu_file, MF_STRING, MENU_ITEM_ID_EXIT, TEXT("E&xit"));
    AppendMenu(g_menu, MF_POPUP, (UINT_PTR)menu_file, "File");
    HMENU menu_view = CreatePopupMenu();
    AppendMenu(menu_view, MF_STRING, MENU_ITEM_ID_DRAW_WIREFRAME, "Draw wireframe\tF2");
    AppendMenu(menu_view, MF_STRING | MF_CHECKED, MENU_ITEM_ID_DRAW_POINTS, "Draw points\tF3");
    AppendMenu(menu_view, MF_STRING | MF_CHECKED, MENU_ITEM_ID_DRAW_FACES, "Draw faces\tF4");
    AppendMenu(menu_view, MF_STRING | MF_CHECKED, MENU_ITEM_ID_DRAW_TRANSPARENCY, "Draw transparency\tF5");
    AppendMenu(g_menu, MF_POPUP, (UINT_PTR)menu_view, "View");
    HMENU menu_help = CreatePopupMenu();
    AppendMenu(menu_help, MF_STRING, MENU_ITEM_ID_HELP, "About\tF1");
    AppendMenu(g_menu, MF_POPUP, (UINT_PTR)menu_help, "Help");
    SetMenu(window, g_menu);

    // Create statusbar
    g_statusbar = CreateStatusWindow(WS_CHILD | WS_VISIBLE, NULL, window, IDC_STATUSBAR);
    if (g_statusbar == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    int parts[] = { -1 };
    SendMessage(g_statusbar, SB_SETPARTS, ARRAYSIZE(parts), (LPARAM)parts);
    SendMessage(g_statusbar, SB_SETTEXT, MAKEWPARAM(0, 0), (LPARAM)&TEXT("Mesh not loaded"));

    // Allow to drag and drop files to window
    DragAcceptFiles(window, TRUE);

    return TRUE;
}

void ShowHelp(HWND window)
{
    MessageBox(window, TEXT(APP_NAME"\n\nFor the latest updates, visit: https://github.com/koshkokoshka/pathologic-mesh-viewer"), TEXT("Help"), MB_OK | MB_ICONINFORMATION);
}

void Window_OnKeyDown(HWND window, WPARAM w_param)
{
    switch (w_param) {
        case VK_F1: ShowHelp(window); break;
        case VK_F2: WindowMenu_ToggleItem(g_menu, MENU_ITEM_ID_DRAW_WIREFRAME); break;
        case VK_F3: WindowMenu_ToggleItem(g_menu, MENU_ITEM_ID_DRAW_POINTS); break;
        case VK_F4: WindowMenu_ToggleItem(g_menu, MENU_ITEM_ID_DRAW_FACES); break;
        case VK_F5: WindowMenu_ToggleItem(g_menu, MENU_ITEM_ID_DRAW_TRANSPARENCY); break;
        case VK_LEFT: {
            char path[MAX_PATH];
            int prev_file_number = g_mesh_directory_file_number-1;
            if (prev_file_number < 0) {
                prev_file_number = g_mesh_directory_files_count-1; // loop directory
            }
            wsprintf(path, "%s\\%s", g_mesh_directory, g_mesh_directory_files[prev_file_number].name);
            TryLoadMesh(window, path);
            break;
        }
        case VK_RIGHT: {
            char path[MAX_PATH];
            int next_file_number = g_mesh_directory_file_number+1;
            if (next_file_number > g_mesh_directory_files_count-1) {
                next_file_number = 0; // loop directory
            }
            wsprintf(path, "%s\\%s", g_mesh_directory, g_mesh_directory_files[next_file_number].name);
            TryLoadMesh(window, path);
            break;
        }
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
        case MENU_ITEM_ID_DRAW_TRANSPARENCY:
            WindowMenu_ToggleItem(g_menu, w_param);
            RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
            break;
        case MENU_ITEM_ID_OPEN: {
            TCHAR path[MAX_PATH];
            path[0] = '\0';
            if (FileSelectDialog_Show(path, sizeof(path), window, "Pathologic game model (.mesh)\0*.mesh\0\0")) {
                TryLoadMesh(window, path);
            }
            RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
            break;
        }
        case MENU_ITEM_ID_EXPORT_OBJ: {
            if (!g_mesh_loaded) {
                MessageBox(window, TEXT("To export .obj file you must load .mesh file first"), TEXT("Mesh not loaded"), MB_OK | MB_ICONWARNING);
                return;
            }
            TCHAR path[MAX_PATH];
            path[0] = '\0';
            strcpy(path, PathFindFileName(g_mesh_path));
            PathRemoveExtension(path);
            PathAddExtension(path, ".obj");
            if (FileOpenDialog_Show(path, sizeof(path), window, "Wavefront OBJ (.obj)\0*.obj\0\0")) {
                if (!ExportLoadedMesh(window, path)) {
                    MessageBox(window, TEXT("Failed to export .OBJ file"), TEXT("Export failed"), MB_OK | MB_ICONERROR);
                }
            }
            break;
        }
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

void Window_OnMouseMove(HWND window, WPARAM w_param, LPARAM l_param)
{
    static int g_mouse_x;
    static int g_mouse_y;

    int x = GET_X_LPARAM(l_param);
    int y = GET_Y_LPARAM(l_param);
    if (w_param == MK_LBUTTON) {
        g_camera_rot += (float)(x - g_mouse_x) * 0.01f;
        g_camera_yaw += (float)(y - g_mouse_y) * 0.01f;
        RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
    }
    if (w_param == MK_RBUTTON) {
        g_camera_x += (float)(x - g_mouse_x) * g_camera_z * 0.001f;
        g_camera_y += (float)(y - g_mouse_y) * g_camera_z * 0.001f;
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
    BOOL draw_transparency = WindowMenu_IsItemChecked(g_menu, MENU_ITEM_ID_DRAW_TRANSPARENCY);
    Viewport_Draw(draw_wireframe, draw_points, draw_faces, draw_transparency);

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

    SelectObject(ps.hdc, GetStockObject(DEFAULT_GUI_FONT));
    RECT client_rect;
    GetClientRect(window, &client_rect);
    if (!g_mesh_loaded) {
        if (g_mesh_error) {
            COLORREF old_bk_color = SetBkColor(ps.hdc, RGB(0xFF, 0x00, 0x00));
            COLORREF old_text_color = SetTextColor(ps.hdc, RGB(0xFF, 0xFF, 0xFF));
            DrawText(ps.hdc, TEXT("An error occurred while trying to load the selected model"), -1, &client_rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            SetBkColor(ps.hdc, old_bk_color);
            SetTextColor(ps.hdc, old_text_color);
        } else {
            DrawText(ps.hdc, TEXT("Select Pathologic .mesh with drag-and-drop or File -> Open..."), -1, &client_rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
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

        case WM_SIZE:
            SendMessage(g_statusbar, WM_SIZE, 0, 0); // auto-size statusbar

            RECT client_rect;
            GetClientRect(window, &client_rect);

            g_screen_width = client_rect.right - client_rect.left;
            g_screen_height = client_rect.bottom - client_rect.top;

            static SIZE_T g_screen_buffer_size = 0;

            SIZE_T buffer_size = g_screen_width * g_screen_height;

            if (g_screen_color == NULL || g_screen_depth == NULL) {
                g_screen_color = HeapAlloc(GetProcessHeap(), 0, buffer_size * sizeof(DWORD));
                g_screen_depth = HeapAlloc(GetProcessHeap(), 0, buffer_size * sizeof(float));
                g_screen_buffer_size = buffer_size;
            } else {
                if (buffer_size > g_screen_buffer_size) {
                    g_screen_color = HeapReAlloc(GetProcessHeap(), 0, g_screen_color, buffer_size * sizeof(DWORD));
                    g_screen_depth = HeapReAlloc(GetProcessHeap(), 0, g_screen_depth, buffer_size * sizeof(float));
                    g_screen_buffer_size = buffer_size;
                }
            }
            if (g_screen_color == NULL || g_screen_depth == NULL) { return 1; }

            RedrawWindow(window, NULL, NULL, RDW_INVALIDATE);
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
            SetCapture(window);
            return 0;

        case WM_LBUTTONUP:
            ReleaseCapture();
            return 0;

        case WM_RBUTTONDOWN:
            SetCapture(window);
            return 0;

        case WM_RBUTTONUP:
            ReleaseCapture();
            return 0;

        case WM_MOUSEMOVE:
            Window_OnMouseMove(window, w_param, l_param);
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

    // Create window
    HWND window = CreateWindowEx(
        WS_EX_COMPOSITED, window_class.lpszClassName,
        TEXT(APP_NAME), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
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
