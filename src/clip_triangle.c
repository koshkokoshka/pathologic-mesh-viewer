#include "clip_triangle.h"

#include "draw.h"

int ClipTriangle(IN OUT Vertex vertices[CLIP_MAX_POINTS], float near_clip)
{
    // Calculate dot products and inside flags
    float dot_products[3];
    BOOL inside_flags[3];
    for (int i = 0; i < 3; ++i) {
        dot_products[i] = vertices[i].z - near_clip;
        inside_flags[i] = dot_products[i] > 0.0f;
    }

    // All points inside
    if (inside_flags[0] && inside_flags[1] && inside_flags[2]) {
        return 1;
    }

    // At least one point inside
    Vertex inside_vertices[CLIP_MAX_POINTS];
    int inside_vertices_count = 0;
    for (int i = 0; i < 3; ++i) {
        int next = (i + 1) % 3;

        if (inside_flags[i]) {
            inside_vertices[inside_vertices_count++] = vertices[i];
        }

        if (inside_flags[i] != inside_flags[next]) {
            float t = dot_products[i] / (dot_products[i] - dot_products[next]);
            Vertex_Lerp(&inside_vertices[inside_vertices_count++], vertices[i], vertices[next], t);
        }
    }

    // Create new triangles based on the number of points inside
    if (inside_vertices_count == 3) {
        // - Single triangle
        vertices[0] = inside_vertices[0];
        vertices[1] = inside_vertices[1];
        vertices[2] = inside_vertices[2];
        return 1;
    }
    if (inside_vertices_count == 4) {
        // - First triangle
        vertices[0] = inside_vertices[0];
        vertices[1] = inside_vertices[1];
        vertices[2] = inside_vertices[2];
        // - Second triangle
        vertices[3] = inside_vertices[0];
        vertices[4] = inside_vertices[2];
        vertices[5] = inside_vertices[3];
        return 2;
    }
    return 0;
}
