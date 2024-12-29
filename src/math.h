#pragma once

static void Point_Rotate(float *x, float *y, float sin, float cos)
{
    float rx = (*x) * cos - (*y) * sin;
    float ry = (*x) * sin + (*y) * cos;
    *x = rx;
    *y = ry;
}
