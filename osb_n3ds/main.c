#include "window.h"
#include "sound.h"
#include "client.h"
#include "characters.h"
#include "utils.h"
#include "stage.h"
#include "main_menu.h"
#include "memory.h"
#include "text.h"
#include "sounds.h"
#include "vshader_rigged_shbin.h"
#include "vshader_2d_shbin.h"
#include "vshader_3d_shbin.h"
#include "vshader_3d_textureless_shbin.h"
#include <stdio.h>

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define CLEAR_COLOR 			0x220022FF
#define FOV 					60.0f*(M_PI/180.0f)
#define NEAR 					0.01
#define ASPECT 					400.0f/240.0f
#define MAX_DRAW_QUEUE_RECTS 	32

Shader 	skinnedShader;
Shader 	standard2DShader;
Shader 	standard3DShader;
Shader 	textureless3DShader;

static C3D_BufInfo 		rectsBufInfo;
static Vertex22 		*rectDrawQueue;
static int 				nRectsInDrawQueue;
static C3D_RenderBuf 	rbTop;
static C3D_RenderBuf 	rbBot;
static float 			projView[16];
static float 			proj[16];
static Sound 			sound;
static float 			deltaTime;
static u64 				lastTime;
static Game 			game;

void BindShader(Shader *shader){
	C3D_BindProgram(&shader->program);
	C3D_SetAttrInfo(&shader->attrInfo);
}

void DrawRect(Rect2D screenRect, Rect2D imgRect, C3D_Tex *tex){

	if(nRectsInDrawQueue > MAX_DRAW_QUEUE_RECTS)
		return;

	Utils_Vertex22Rect(&rectDrawQueue[6 * nRectsInDrawQueue], screenRect, imgRect);

	C3D_SetBufInfo(&rectsBufInfo);

    C3D_TexBind(0, tex);
    C3D_DrawArrays(GPU_TRIANGLES, nRectsInDrawQueue * 6, 6);

	++nRectsInDrawQueue;
}

float GetDeltaTime(void){

	return deltaTime;
}

u64 GetCurrTime(void){

	return lastTime;
}

static void Shader_Close(Shader *shader){
	shaderProgramFree(&shader->program);
	DVLB_Free(shader->dvlb);
}

static void Shader_Create(Shader *shader, u32 *vshader, u32 vshaderSize){
	shader->dvlb = DVLB_ParseFile(vshader, vshaderSize);
	shaderProgramInit(&shader->program);
	shaderProgramSetVsh(&shader->program, &shader->dvlb->DVLE[0]);
	C3D_BindProgram(&shader->program);

	shader->modelLoc = shaderInstanceGetUniformLocation(shader->program.vertexSohader, "model");
	shader->projViewLoc = shaderInstanceGetUniformLocation(shader->program.vertexShader, "projView");
	
	AttrInfo_Init(&shader->attrInfo);
}

static void Init(void){

	Memory_Init(0x01 << 20);
	romfsInit();
	csndInit();

	Client_Start();

	// -------------------------------------------

	Shader_Create(&standard2DShader, (u32 *)vshader_2d_shbin, vshader_2d_shbin_size);

	AttrInfo_AddLoader(&standard2DShader.attrInfo, 0, GPU_FLOAT, 2); // v0=position
	AttrInfo_AddLoader(&standard2DShader.attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord

	// -------------------------------------------

	Shader_Create(&standard3DShader, (u32 *)vshader_3d_shbin, vshader_3d_shbin_size);

	AttrInfo_AddLoader(&standard3DShader.attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(&standard3DShader.attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord

	// -------------------------------------------

	Shader_Create(&textureless3DShader, (u32 *)vshader_3d_textureless_shbin, vshader_3d_textureless_shbin_size);

	AttrInfo_AddLoader(&textureless3DShader.attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(&textureless3DShader.attrInfo, 1, GPU_FLOAT, 4); // v1=color

	// -------------------------------------------

	Shader_Create(&skinnedShader, (u32 *)vshader_rigged_shbin, vshader_rigged_shbin_size);

	skinnedShader.bonesLoc = shaderInstanceGetUniformLocation(skinnedShader.program.vertexShader, "boneMatrices");

	AttrInfo_AddLoader(&skinnedShader.attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(&skinnedShader.attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
	AttrInfo_AddLoader(&skinnedShader.attrInfo, 2, GPU_FLOAT, 3); // v3=weights
	AttrInfo_AddLoader(&skinnedShader.attrInfo, 3, GPU_FLOAT, 3); // v4=bones

	// -------------------------------------------

	rectDrawQueue = (Vertex22 *)linearAlloc(6 * MAX_DRAW_QUEUE_RECTS * sizeof(Vertex22));

	BufInfo_Init(&rectsBufInfo);
	BufInfo_Add(&rectsBufInfo, (void *)rectDrawQueue, sizeof(Vec2) * 2, 2, 0x10);

	nRectsInDrawQueue = 0;

	// -------------------------------------------

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	// -------------------------------------------

	Math_Perspective(proj, FOV, ASPECT, NEAR, 1000.0f);

	// --------------------------------------------

	Sound_Load(&sound, SOUND_TEST);

	lastTime = osGetTime();

	Text_Init();

	MainMenu_Init(&game);

	// Stage_Create(STAGE_SPACE);
	// Stage_AddPlayer(CHARACTER_MOTAVIO);

	// Stage_SetDrawDebugPlanes(STAGE_DEBUG);
}

static void Flush(void){
	C3D_Flush();
	nRectsInDrawQueue = 0;
	Text_Clear();
}

static void Render(void){

	// float view[16];
	// Stage_UpdateCamera(view, FOV, NEAR, ASPECT);

	// Math_MatrixMatrixMult(projView, proj, view);

	// C3D_BindProgram(&textureless3DShader.program);
	// Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, textureless3DShader.projViewLoc, 4), projView);
	// C3D_BindProgram(&skinnedShader.program);
	// Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, skinnedShader.projViewLoc, 4), projView);
	// C3D_BindProgram(&standard3DShader.program);
	// Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standard3DShader.projViewLoc, 4), projView);

	C3D_RenderBufBind(&rbTop);
	game.RenderTop(&game);
	// Stage_RenderTop();
	Flush();

	C3D_RenderBufBind(&rbBot);
	game.RenderBottom(&game);
	// Stage_RenderBottom();
	Flush();
}

static void Update(void){

	u64 currTime = osGetTime();

	deltaTime = (float)(currTime - lastTime) / 1000.0f;
		
	lastTime = currTime;

	Client_KeepAlive();

	// game.Update(&game);

	// Stage_Update();
}

static void Exit(void){

	Memory_Close();


	Shader_Close(&skinnedShader);
	Shader_Close(&textureless3DShader);
	Shader_Close(&standard3DShader);

	linearFree(rectDrawQueue);

	Sound_Close(&sound);

	Text_Close();

	game.Close(&game);

	Client_Close();

	// Stage_Close();
	// Shader_Close(&standard2DShader);
	csndExit();
	socExit();
	romfsExit();
}

int main(){

	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	C3D_RenderBufInit(&rbBot, BOTTOM_HEIGHT, BOTTOM_WIDTH, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderBufInit(&rbTop, TOP_HEIGHT, TOP_WIDTH, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	rbTop.clearColor = CLEAR_COLOR;
	rbBot.clearColor = CLEAR_COLOR;
	C3D_RenderBufClear(&rbTop);
	C3D_RenderBufClear(&rbBot);

	Init();

	Sound_Play(0x8, &sound);

	while (aptMainLoop())
	{

		C3D_VideoSync();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break;

		Update();

		Render();

		C3D_RenderBufTransfer(&rbTop, (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), DISPLAY_TRANSFER_FLAGS);
		C3D_RenderBufClear(&rbTop);

		C3D_RenderBufTransfer(&rbBot, (u32*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), DISPLAY_TRANSFER_FLAGS);
		C3D_RenderBufClear(&rbBot);
	}

	Exit();
	C3D_Fini();
	gfxExit();
	return 0;
}