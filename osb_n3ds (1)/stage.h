#ifndef STAGE_DEF
#define STAGE_DEF

#include <3ds.h>
#include "math.h"

#define MAX_PLAYERS 4

enum {
	STAGE_NO_DEBUG = 0,
	STAGE_DEBUG
};

enum {
	STAGE_SPACE = 0,
	NUM_STAGES,
};

void Stage_Create(u8 stage);
void Stage_AddPlayer(u8 index);
void Stage_Close(void);
void Stage_RenderTop(void);
void Stage_RenderBottom(void);
void Stage_Update(void);
void Stage_DrawShadow(Vec3 pos, Vec2 size);
void Stage_SetDrawDebugPlanes(u8 set);
void Stage_UpdateCamera(float *view, float fov, float near, float aspect);

#endif