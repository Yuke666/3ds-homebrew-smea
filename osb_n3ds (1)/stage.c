#include "stage.h"
#include "window.h"
#include "utils.h"
#include "model.h"
#include "characters.h"

#define MAX_STAGE_PLATFORMS 6
#define MIN_CAMERA_SCALE 6
#define CAMERA_HEIGHT 0.4
#define CAMERA_PADDING 0.5

typedef struct {
	float pos[3];
	float color[4];
} Vertex34;

static struct {

	const char path[16];

	Rect2D platforms[MAX_STAGE_PLATFORMS];

	u8 nPlatforms;

} stages[] = {

	{ "stage.n3ds" , {
			(Rect2D){-2, 1, 4, 0.1},
			(Rect2D){-1.7, 2, 1.1, 0.1},
			(Rect2D){-0.5, 2.74, 1, 0.15},
			(Rect2D){0.65, 2, 1.1, 0.1},
		}, 4
	},
};

static u8 drawDebugCubes = 0;

static int nStageStackAllocations;

static Character players[MAX_PLAYERS]; 
static u8 nPlayers;
static u8 stageIndex;
static Model levelModel;

static Vec3 currCamPos;
static Vec3 currCenter;

static Vertex34 *cubeVboData;
static C3D_BufInfo cubeBufInfo;

static Vertex34 cubeVerts[] = {
	// First face (PZ)
	(Vertex34){ {0.0f, 0.0f, 1.0f}, {1,0,0,0.4}},
	(Vertex34){ {1.0f, 0.0f, 1.0f}, {1,0,0,0.4}},
	(Vertex34){ {1.0f, 1.0f, 1.0f}, {1,0,0,0.4}},
	(Vertex34){ {1.0f, 1.0f, 1.0f}, {1,0,0,0.4}},
	(Vertex34){ {0.0f, 1.0f, 1.0f}, {1,0,0,0.4}},
	(Vertex34){ {0.0f, 0.0f, 1.0f}, {1,0,0,0.4}},
	// Second face (MZ)
	(Vertex34){ {0.0f, 0.0f, 0.0f}, {0,0,1,0.4}},
	(Vertex34){ {0.0f, 1.0f, 0.0f}, {0,0,1,0.4}},
	(Vertex34){ {1.0f, 1.0f, 0.0f}, {0,0,1,0.4}},
	(Vertex34){ {1.0f, 1.0f, 0.0f}, {0,0,1,0.4}},
	(Vertex34){ {1.0f, 0.0f, 0.0f}, {0,0,1,0.4}},
	(Vertex34){ {0.0f, 0.0f, 0.0f}, {0,0,1,0.4}},
	// Third face (PX)
	(Vertex34){ {1.0f, 0.0f, 0.0f}, {1,1,0,0.4}},
	(Vertex34){ {1.0f, 1.0f, 0.0f}, {1,1,0,0.4}},
	(Vertex34){ {1.0f, 1.0f, 1.0f}, {1,1,0,0.4}},
	(Vertex34){ {1.0f, 1.0f, 1.0f}, {1,1,0,0.4}},
	(Vertex34){ {1.0f, 0.0f, 1.0f}, {1,1,0,0.4}},
	(Vertex34){ {1.0f, 0.0f, 0.0f}, {1,1,0,0.4}},
	// Fourth face (MX)
	(Vertex34){ {0.0f, 0.0f, 0.0f}, {1,0,1,0.4}},
	(Vertex34){ {0.0f, 0.0f, 1.0f}, {1,0,1,0.4}},
	(Vertex34){ {0.0f, 1.0f, 1.0f}, {1,0,1,0.4}},
	(Vertex34){ {0.0f, 1.0f, 1.0f}, {1,0,1,0.4}},
	(Vertex34){ {0.0f, 1.0f, 0.0f}, {1,0,1,0.4}},
	(Vertex34){ {0.0f, 0.0f, 0.0f}, {1,0,1,0.4}},
	// Fifth face (PY)
	(Vertex34){ {0.0f, 1.0f, 0.0f}, {0,1,1,0.4}},
	(Vertex34){ {0.0f, 1.0f, 1.0f}, {0,1,1,0.4}},
	(Vertex34){ {1.0f, 1.0f, 1.0f}, {0,1,1,0.4}},
	(Vertex34){ {1.0f, 1.0f, 1.0f}, {0,1,1,0.4}},
	(Vertex34){ {1.0f, 1.0f, 0.0f}, {0,1,1,0.4}},
	(Vertex34){ {0.0f, 1.0f, 0.0f}, {0,1,1,0.4}},
	// Sixth face (MY)
	(Vertex34){ {0.0f, 0.0f, 0.0f}, {0,1,0,0.4}},
	(Vertex34){ {1.0f, 0.0f, 0.0f}, {0,1,0,0.4}},
	(Vertex34){ {1.0f, 0.0f, 1.0f}, {0,1,0,0.4}},
	(Vertex34){ {1.0f, 0.0f, 1.0f}, {0,1,0,0.4}},
	(Vertex34){ {0.0f, 0.0f, 1.0f}, {0,1,0,0.4}},
	(Vertex34){ {0.0f, 0.0f, 0.0f}, {0,1,0,0.4}},
};

static Vertex34 *shadowVboData;
static C3D_BufInfo shadowBufInfo;

static Vertex34 shadowVerts[] = {
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, -1.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.382683, 0.000000, -0.923880}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.382683, 0.000000, -0.923880}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.707107, 0.000000, -0.707107}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.707107, 0.000000, -0.707107}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.923880, 0.000000, -0.382683}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.923880, 0.000000, -0.382683}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-1.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-1.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.923880, 0.000000, 0.382684}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.923880, 0.000000, 0.382684}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.707107, 0.000000, 0.707107}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.707107, 0.000000, 0.707107}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.382683, 0.000000, 0.923880}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.382683, 0.000000, 0.923880}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.000000, 0.000000, 1.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {-0.000000, 0.000000, 1.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.382683, 0.000000, 0.923880}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.382683, 0.000000, 0.923880}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.707107, 0.000000, 0.707107}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.707107, 0.000000, 0.707107}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.923880, 0.000000, 0.382684}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.923880, 0.000000, 0.382684}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {1.000000, 0.000000, -0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {1.000000, 0.000000, -0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.923879, 0.000000, -0.382684}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.923879, 0.000000, -0.382684}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.707107, 0.000000, -0.707107}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.707107, 0.000000, -0.707107}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.382683, 0.000000, -0.923880}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, 0.000000}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.382683, 0.000000, -0.923880}, {0.05, 0.05, 0.05, 0.5}},
	(Vertex34){ {0.000000, 0.000000, -1.000000}, {0.05, 0.05, 0.05, 0.5}},
};

void Stage_Create(u8 stage){

	nStageStackAllocations = 0;

	// load level
	stageIndex = stage;
	Model_Load(&levelModel, stages[stage].path);

	// shadow buffer
	shadowVboData = (Vertex34 *)Memory_StackAlloc(MAIN_STACK, sizeof(shadowVerts));
	memcpy(shadowVboData, shadowVerts, sizeof(shadowVerts));
	BufInfo_Init(&shadowBufInfo);
	BufInfo_Add(&shadowBufInfo, shadowVboData, sizeof(Vertex34), 2, 0x10);
	
	++nStageStackAllocations;
	
	// cube buffer
	cubeVboData = (Vertex34 *)Memory_StackAlloc(MAIN_STACK, sizeof(cubeVerts));
	memcpy(cubeVboData, cubeVerts, sizeof(cubeVerts));
	BufInfo_Init(&cubeBufInfo);
	BufInfo_Add(&cubeBufInfo, cubeVboData, sizeof(Vertex34), 2, 0x10);

	++nStageStackAllocations;

	currCenter = (Vec3){0,2,0};
	currCamPos = (Vec3){0, 2.7, 3.5};
}

void Stage_SetDrawDebugPlanes(u8 set){
	
	drawDebugCubes = set;
}

void Stage_AddPlayer(u8 index){

	Character_Create(&players[nPlayers], index);

	++nPlayers;
}

void Stage_UpdateCamera(float *view, float fov, float near, float aspect){

	float minX = HUGE_VAL;
	float maxX = -HUGE_VAL;
	float minY = HUGE_VAL;
	float maxY = -HUGE_VAL;

	int k;
	for(k = 0; k < nPlayers; k++){
		if(players[k].boundingBox.x < minX)
			minX = players[k].boundingBox.x;
		if(players[k].boundingBox.x+players[k].boundingBox.w > maxX)
			maxX = players[k].boundingBox.x+players[k].boundingBox.w;
		if(players[k].boundingBox.y < minY)
			minY = players[k].boundingBox.y;
		if(players[k].boundingBox.y+players[k].boundingBox.h > maxY)
			maxY = players[k].boundingBox.y+players[k].boundingBox.h;
	}

	minX -= CAMERA_PADDING;
	minY -= CAMERA_PADDING;
	maxX += CAMERA_PADDING;
	maxY += CAMERA_PADDING;

	float width = maxX - minX;
	float height = maxY - minY;

	float scale = sqrtf(pow(width, 2) + pow(height, 2));

	scale = MAX(MIN_CAMERA_SCALE, scale);
	
	Vec3 center = (Vec3){ minX + (width/2), minY + (height/2) + CAMERA_HEIGHT, 0 };

	Vec3 toCamPos;

	toCamPos.x = center.x;
	toCamPos.y = center.y;
	toCamPos.z = near + (scale / (2 * tan(fov/2) * aspect));

	currCenter = Math_LerpVec3(currCenter, center, GetDeltaTime() * 1);
	currCamPos = Math_LerpVec3(currCamPos, toCamPos, GetDeltaTime() * 1);

	Math_LookAt(view, currCamPos, currCenter, (Vec3){0,1,0});
}

void Stage_Close(void){

	int k;
	for(k = nPlayers-1; k >= 0; k--) // start at last loaded.
		players[k].Free(&players[k]);

	Model_Free(&levelModel);

	Memory_StackPop(MAIN_STACK, nStageStackAllocations);
}

static void DrawCube(Rect2D rect, float depth, float z){

	float model[16], scale[16];

	Math_TranslateMatrix(model, (Vec3){rect.x, rect.y, z});

	Math_ScalingMatrixXYZ(scale, rect.w, rect.h, depth);

	Math_MatrixMatrixMult(model, model, scale);

	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, textureless3DShader.modelLoc, 4), model);

	C3D_DrawArrays(GPU_TRIANGLES, 0, 36);
}

void Stage_RenderTop(void){

	float model[16];
	Math_Identity(model);

	BindShader(&standard3DShader);
	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standard3DShader.modelLoc, 4), model);

	Model_Draw(&levelModel);

	int k;
	for(k = 0; k < nPlayers; k++)
		players[k].Render(&players[k]);

	if(drawDebugCubes){

		Rect2D *platforms = stages[stageIndex].platforms;

		BindShader(&textureless3DShader);
		C3D_SetBufInfo(&cubeBufInfo);
		
		C3D_TexEnv* env = C3D_GetTexEnv(0);
		C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, 0);

		for(k = 0; k < stages[stageIndex].nPlatforms; k++)
			DrawCube(platforms[k], 1.5, -1);

		for(k = 0; k < nPlayers; k++)
			DrawCube(players[k].boundingBox, 0.5, -0.15);
	
		env = C3D_GetTexEnv(0);
		C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
	}
}

void Stage_RenderBottom(void){

}

void Stage_Update(void){

	int k;
	for(k = 0; k < nPlayers; k++)
		players[k].Update(&players[k]);
}

void Stage_RayCast(Vec3 pos, Vec3 dir){

}

void Stage_DrawShadow(Vec3 pos, Vec2 size){

	float trans[16], matrix[16];
	Math_TranslateMatrix(trans, pos);
	Math_ScalingMatrixXYZ(matrix, size.x, 0, size.y);

	Math_MatrixMatrixMult(matrix, trans, matrix);

	// --------------------------------------------

	BindShader(&textureless3DShader);
	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, textureless3DShader.modelLoc, 4), matrix);

	C3D_SetBufInfo(&shadowBufInfo);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);

	// C3D_FVec *vec = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standard3DShader.uColorLoc, 1);

	// *vec = (C3D_FVec){ .x = 0.05, .y = 0.05, .z = 0.05, .w = 0.5 };

	C3D_DrawArrays(GPU_TRIANGLES, 0, 48);
	
	// *vec = (C3D_FVec){ .x = 1, .y = 1, .z = 1, .w = 1 };

	env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
}