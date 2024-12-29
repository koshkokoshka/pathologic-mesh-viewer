#include "clip_line.h"

#include <math.h>

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
