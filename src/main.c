#include <windows.h>
#include <windowsx.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <shlwapi.h>
#include <stdio.h>

#include "math.h"
#include "draw.h"
#include "mesh.h"

// Window menu
#define MENU_ITEM_ID_DRAW_WIREFRAME 1
#define MENU_ITEM_ID_DRAW_POINTS 2
#define MENU_ITEM_ID_DRAW_FACES 3
#define MENU_ITEM_ID_OPEN 10
#define MENU_ITEM_ID_EXIT 20
#define MENU_ITEM_ID_HELP 30
HMENU g_menu;

// Loaded mesh
Mesh g_mesh;
BOOL g_mesh_loaded = FALSE;
BOOL g_mesh_error = FALSE;
char g_mesh_path[MAX_PATH];

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

void TryLoadMesh(HWND window, const char *path)
{
    strcpy(g_mesh_path, path);
    g_mesh_loaded = Mesh_LoadFromPathologicFormat(&g_mesh, path);
    if (!g_mesh_loaded) {
        g_mesh_error = TRUE;
        return;
    }
    Viewport_CalculateBounds();
}

void Viewport_Draw(BOOL draw_wireframe, BOOL draw_points, BOOL draw_faces)
{
    // Precompute screen-related values
    PrecomputeRenderingVariables();

    // Clear buffers
    for (int i = 0; i < g_screen_width * g_screen_height; ++i) { g_screen_color[i] = 0x393029; }
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
                DrawTriangle(submesh.texture,
                             ax, ay, az, a.u, a.v,
                             bx, by, bz, b.u, b.v,
                             cx, cy, cz, c.u, c.v);
            }
            if (draw_wireframe) {
                DrawLine(ax, ay, bx, by, 0x999922);
                DrawLine(bx, by, cx, cy, 0x999922);
                DrawLine(cx, cy, ax, ay, 0x999922);
            }
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
    AppendMenu(g_menu, MF_POPUP, (UINT_PTR)menu_view, "View");
    HMENU menu_help = CreatePopupMenu();
    AppendMenu(menu_help, MF_STRING, MENU_ITEM_ID_HELP, "About\tF1");
    AppendMenu(g_menu, MF_POPUP, (UINT_PTR)menu_help, "Help");
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
    Viewport_Draw(draw_wireframe, draw_points, draw_faces);

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
            DrawText(ps.hdc, TEXT("Select Pathologic .mesh with File -> Open..."), -1, &client_rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
    }
    if (g_mesh_path != NULL) {
        DrawText(ps.hdc, g_mesh_path, -1, &client_rect, DT_RIGHT);
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
        WS_EX_COMPOSITED, window_class.lpszClassName, TEXT("Pathologic Mesh Viewer v0.3"), window_style,
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
