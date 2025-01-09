#include "draw.h"

#include <math.h>

void DrawTriangle(Texture texture, BOOL transparency,
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

    // Clip against screen bounds
    if (min_x < 0) { min_x = 0; }
    if (max_x > g_screen_width) { max_x = g_screen_width; }
    if (min_y < 0) { min_y = 0; }
    if (max_y > g_screen_height) { max_y = g_screen_height; }

    // Compute edge coefficients
    const int a12 = y2 - y3;
    const int b12 = x3 - x2;
    const int a23 = y3 - y1;
    const int b23 = x1 - x3;
    const int a31 = y1 - y2;
    const int b31 = x2 - x1;

    // Precompute initial edge values
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

    // Precompute texture size values
    const int tw = texture.width;
    const int th = texture.height;
    const int tw_mask = tw - 1;
    const int th_mask = th - 1;
    const BOOL is_power_of_two = (tw & (tw - 1)) == 0 && (th & (th - 1)) == 0;

    // Precompute Z interpolation values
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

                    // Faster texture coordinate wrapping for power-of-two textures
                    int tx, ty;
                    if (is_power_of_two) {
                        tx = ((int)(u * tw)) & tw_mask;
                        ty = ((int)(v * th)) & th_mask;
                    } else {
                        u = u - floorf(u);
                        v = v - floorf(v);
                        tx = (int)(u * tw);
                        ty = (int)(v * th);
                        tx = tx < 0 ? 0 : (tx >= tw ? tw - 1 : tx);
                        ty = ty < 0 ? 0 : (ty >= th ? th - 1 : ty);
                    }
                    DWORD color = texture.data[(ty * tw) + tx];
                    if (transparency == 0 || (color & 0xFF000000) != 0) { // alpha-test
                        g_screen_color[i] = color;
                        g_screen_depth[i] = z;
                    }
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
