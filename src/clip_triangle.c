#include "clip_triangle.h"

#include "math.h"
#include "mesh.h"

static void MeshPoint_Lerp(OUT MeshPoint *out, MeshPoint a, MeshPoint b, float t)
{
    out->x = Lerp(a.x, b.x, t);
    out->y = Lerp(a.y, b.y, t);
    out->z = Lerp(a.z, b.z, t);
    out->u = Lerp(a.u, b.u, t);
    out->v = Lerp(a.v, b.v, t);
}

int ClipTriangle(IN OUT MeshPoint points[CLIP_MAX_POINTS], float near_clip)
{
    // Calculate dot products and inside flags
    float dot_products[3];
    BOOL inside_flags[3];
    for (int i = 0; i < 3; ++i) {
        dot_products[i] = points[i].z - near_clip;
        inside_flags[i] = dot_products[i] > 0.0f;
    }

    // All points inside
    if (inside_flags[0] && inside_flags[1] && inside_flags[2]) {
        return 1;
    }

    // At least one point inside
    MeshPoint points_inside[CLIP_MAX_POINTS];
    int points_i = 0;
    for (int i = 0; i < 3; ++i) {
        int next = (i + 1) % 3;

        if (inside_flags[i]) {
            points_inside[points_i++] = points[i];
        }

        if (inside_flags[i] != inside_flags[next]) {
            float t = dot_products[i] / (dot_products[i] - dot_products[next]);
            MeshPoint_Lerp(&points_inside[points_i++], points[i], points[next], t);
        }
    }

    // Create new triangles based on the number of points inside
    if (points_i == 3) {
        // - Single triangle
        points[0] = points_inside[0];
        points[1] = points_inside[1];
        points[2] = points_inside[2];
        return 1;
    }
    if (points_i == 4) {
        // - First triangle
        points[0] = points_inside[0];
        points[1] = points_inside[1];
        points[2] = points_inside[2];
        // - Second triangle
        points[3] = points_inside[0];
        points[4] = points_inside[2];
        points[5] = points_inside[3];
        return 2;
    }
    return 0;
}
