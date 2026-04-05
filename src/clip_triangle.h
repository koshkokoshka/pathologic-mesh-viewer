#pragma once

#include <windows.h>
#include "draw.h"

#define CLIP_MAX_POINTS 6 // max number of points that can be output by the clip triangle function

int ClipTriangle(IN OUT Vertex vertices[CLIP_MAX_POINTS], float near_clip);
