#include "draw.h"

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
        DrawPoint_Unsafe(x1, y1, color);

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
