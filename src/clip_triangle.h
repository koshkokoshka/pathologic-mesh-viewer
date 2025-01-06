#pragma once

#include <windows.h>
#include "mesh.h"

#define CLIP_MAX_POINTS 6 // max number of points that can be output by the clip triangle function

int ClipTriangle(IN OUT MeshPoint points[CLIP_MAX_POINTS], float near_clip);
