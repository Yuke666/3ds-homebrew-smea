#ifndef UTILS_DEF
#define UTILS_DEF

#include <3ds.h>
#include <citro3d.h>

#include "math.h"

typedef struct {
    Vec2 pos;
    Vec2 coord;
} Vertex22;

void Utils_LoadImage(C3D_Tex *tex, const char *path, int filter, int channels);
void Utils_SetUniformMatrix(C3D_FVec *rows, float *matrix);
void Utils_Vertex22Rect(Vertex22 *out, Rect2D screenRect, Rect2D imgRect);

#endif