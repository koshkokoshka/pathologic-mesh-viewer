#include "draw.h"

#include <math.h>

static void SwapFloat(float* a, float* b) { float c = *a; *a = *b; *b = c; }

static DWORD Texture_SampleNearest(const Texture *texture, float u, float v)
{
    // Wrap UV coordinates and convert to texture space
    const int tx = (int)((float)texture->width  * (u - floorf(u)));
    const int ty = (int)((float)texture->height * (v - floorf(v)));

    // Sample texture color
    return texture->data[ty * texture->width + tx];
}

void DrawTriangle_Span(
    const Texture *texture,
    BOOL transparency,
    int y,
    float lx, float lz, float la, float lb, // left edge
    float rx, float rz, float ra, float rb  // right edge
) {
    // Ensure "l" is always to the left of "r"
    if (lx > rx) {
        SwapFloat(&lx, &rx);
        SwapFloat(&lz, &rz);
        SwapFloat(&la, &ra);
        SwapFloat(&lb, &rb);
    }

    // Precalculate spans
    float span_x = rx - lx;
    if (span_x <= 0.0f) {
        return; // skip if no pixels to draw
    }
    float inv_span_x = 1.0f / span_x;
    float span_z = rz - lz;
    float span_a = ra - la;
    float span_b = rb - lb;

    // Round x-coordinates and clamp to screen bounds
    int ilx = (int) ceilf(lx - 0.5f); // top-left rule: ceil to include left edge
    int irx = (int)floorf(rx - 0.5f); // top-left rule: floor to exclude right edge
    int x_min = max(ilx, 0);
    int x_max = min(irx + 1, g_screen_width);

    // Precompute interpolant steps
    float dz_step = span_z * inv_span_x;
    float da_step = span_a * inv_span_x;
    float db_step = span_b * inv_span_x;

    // Initial interpolant values
    float t = ((float)x_min + 0.5f - lx) * inv_span_x;
    float z = lz + span_z * t;
    float a = la + span_a * t;
    float b = lb + span_b * t;

    // Get scanline pointers
    int scanline = (g_screen_width * y) + x_min;
    DWORD *color_ptr = &g_screen_color[scanline];
    float *depth_ptr = &g_screen_depth[scanline];

    // Loop over the horizontal range of the current scanline
    for (int x = x_min; x < x_max; x++) {

        // Depth test
        if (z <= *depth_ptr) {
            goto DISCARD;
        }
        float inv_z = 1.0f / z; // precompute 1/z for perspective-correct interpolation

        // Perspective-correct texture interpolation
        float u = a * inv_z;
        float v = b * inv_z;

        // Sample color based on the texture filtering method
        DWORD color = Texture_SampleNearest(texture, u, v);

        // Transparency-bit test
        if (transparency && (color & 0xFF000000) == 0) {
            goto DISCARD;
        }

        *color_ptr = color;
        *depth_ptr = z;

    DISCARD:
        // Advance interpolants
        z += dz_step;
        a += da_step;
        b += db_step;

        // Advance scanline pointers
        color_ptr++;
        depth_ptr++;
    }
}

void DrawTriangle(
    const Texture *texture, BOOL transparency,
    float x1, float y1, float z1, float a1, float b1,
    float x2, float y2, float z2, float a2, float b2,
    float x3, float y3, float z3, float a3, float b3
) {
    // Sort vertices by y-coordinate ascending (y1 <= y2 <= y3)
    if (y1 > y2) {
        SwapFloat(&x1, &x2);
        SwapFloat(&y1, &y2);
        SwapFloat(&z1, &z2);
        SwapFloat(&a1, &a2);
        SwapFloat(&b1, &b2);
    }
    if (y1 > y3) {
        SwapFloat(&x1, &x3);
        SwapFloat(&y1, &y3);
        SwapFloat(&z1, &z3);
        SwapFloat(&a1, &a3);
        SwapFloat(&b1, &b3);
    }
    if (y2 > y3) {
        SwapFloat(&x2, &x3);
        SwapFloat(&y2, &y3);
        SwapFloat(&z2, &z3);
        SwapFloat(&a2, &a3);
        SwapFloat(&b2, &b3);
    }

    // Round y-coordinates and clamp to screen bounds
    int y_min = max(    (int)ceilf(y1 - 0.5f), 0);
    int y_mid = min(max((int)ceilf(y2 - 0.5f), 0), g_screen_height);
    int y_max = min(    (int)ceilf(y3 - 0.5f),     g_screen_height);

    if (y_min >= y_max) {
        return; // skip if no pixels to draw
    }

    // Prepare the triangle vertices for perspective-correct texture interpolation
    z1 = 1.0f / z1;
    z2 = 1.0f / z2;
    z3 = 1.0f / z3;

    a1 *= z1;
    b1 *= z1;

    a2 *= z2;
    b2 *= z2;

    a3 *= z3;
    b3 *= z3;

    // Precalculate edges
    float dy13 = y3 - y1;
    float dx13 = x3 - x1;
    float dz13 = z3 - z1;
    float da13 = a3 - a1;
    float db13 = b3 - b1;

    float dy12 = y2 - y1;
    float dx12 = x2 - x1;
    float dz12 = z2 - z1;
    float da12 = a2 - a1;
    float db12 = b2 - b1;

    float dy23 = y3 - y2;
    float dx23 = x3 - x2;
    float dz23 = z3 - z2;
    float da23 = a3 - a2;
    float db23 = b3 - b2;

    // Draw the top part of the triangle
    for (int y = y_min; y < y_mid; y++) {
        float pixel_center = (float)y + 0.5f;
        float lt = (pixel_center - y1) / dy13;
        float rt = (pixel_center - y1) / dy12;
        DrawTriangle_Span(
            texture, transparency, y,
            x1 + dx13 * lt,
            z1 + dz13 * lt,
            a1 + da13 * lt,
            b1 + db13 * lt,
            x1 + dx12 * rt,
            z1 + dz12 * rt,
            a1 + da12 * rt,
            b1 + db12 * rt
        );
    }

    // Draw the bottom part of the triangle
    for (int y = y_mid; y < y_max; y++) {
        float pixel_center = (float)y + 0.5f;
        float lt = (pixel_center - y1) / dy13;
        float rt = (pixel_center - y2) / dy23;
        DrawTriangle_Span(
            texture, transparency, y,
            x1 + dx13 * lt,
            z1 + dz13 * lt,
            a1 + da13 * lt,
            b1 + db13 * lt,
            x2 + dx23 * rt,
            z2 + dz23 * rt,
            a2 + da23 * rt,
            b2 + db23 * rt
        );
    }
}
