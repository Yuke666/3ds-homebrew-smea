#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "vshader_rigged_shbin.h"
#include "vshader_shbin.h"
#include "model.h"
#include "sound.h"
// #include "motavio_bin.h"

typedef struct {
	shaderProgram_s program;
	DVLB_s *dvlb;
	int projViewLoc;
	int modelLoc;
} Shader;

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define CLEAR_COLOR 0x0f0034FF

static Shader skinnedShader;
static Shader standardShader;

static int boneMatricesLoc;
static C3D_Mtx modelMatrix;
static float characterMatrix[16];
static float projView[16];

static Model model, stageModel, shadowModel;
static float cTime = 0;
static int nAnimations = 0;
static Skeleton skeleton;
static Animation animation;


static void Init(void){
	
	// -------------------------------------------

	standardShader.dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&standardShader.program);
	shaderProgramSetVsh(&standardShader.program, &standardShader.dvlb->DVLE[0]);
	C3D_BindProgram(&standardShader.program);

	standardShader.modelLoc = shaderInstanceGetUniformLocation(standardShader.program.vertexShader, "model");
	standardShader.projViewLoc = shaderInstanceGetUniformLocation(standardShader.program.vertexShader, "projView");

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=positino
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord

	// -------------------------------------------

	skinnedShader.dvlb = DVLB_ParseFile((u32*)vshader_rigged_shbin, vshader_rigged_shbin_size);
	shaderProgramInit(&skinnedShader.program);
	shaderProgramSetVsh(&skinnedShader.program, &skinnedShader.dvlb->DVLE[0]);
	C3D_BindProgram(&skinnedShader.program);

	skinnedShader.modelLoc = shaderInstanceGetUniformLocation(skinnedShader.program.vertexShader, "model");
	skinnedShader.projViewLoc = shaderInstanceGetUniformLocation(skinnedShader.program.vertexShader, "projView");
	boneMatricesLoc = shaderInstanceGetUniformLocation(skinnedShader.program.vertexShader, "boneMatrices");

	attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=positino
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
	AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3); // v3=weights
	AttrInfo_AddLoader(attrInfo, 3, GPU_FLOAT, 3); // v4=bones

	// -------------------------------------------

	float view[16];
	Math_Perspective(projView, 60.0f*(M_PI/180.0f), 400.0f/240.0f, 0.01f, 1000.0f);
	Math_LookAt(view, (Vec3){0,2.7,3.3}, (Vec3){0,2.55,2.3}, (Vec3){0,1,0});

	Math_MatrixMatrixMult(projView, projView, view);

	Mtx_Identity(&modelMatrix);

	float trans[16];
	Math_TranslateMatrix(trans, (Vec3){0, 1.1, 0.1});
	Math_ScalingMatrixXYZ(characterMatrix, 0.25, 0.25, 0.25);
	Math_MatrixMatrixMult(characterMatrix, trans, characterMatrix);

	// RiggedModel_Load(&model, &skeleton, &animation, &nAnimations, motavio_bin);
	Model_Load(&stageModel, "stage.n3ds");
	Model_Load(&shadowModel, "shadow.n3ds");
	RiggedModel_Load(&model, &skeleton, &animation, &nAnimations, "motavio.n3ds");
	
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

}

static void SetMatrix(C3D_FVec *rows, float *matrix){

	int k;
	for(k = 0; k < 4; k++){
		rows[k].x = matrix[(k*4)];
		rows[k].y = matrix[(k*4)+1];
		rows[k].z = matrix[(k*4)+2];
		rows[k].w = matrix[(k*4)+3];
	}
}

static void Render(void){

	C3D_BindProgram(&standardShader.program);
	SetMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standardShader.projViewLoc, 4), projView);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, standardShader.modelLoc, &modelMatrix);

	Model_Draw(&stageModel);

	SetMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standardShader.modelLoc, 4), characterMatrix);

	Model_Draw(&shadowModel);


	C3D_BindProgram(&skinnedShader.program);
	SetMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, skinnedShader.projViewLoc, 4), projView);
	SetMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, skinnedShader.modelLoc, 4), characterMatrix);

	C3D_FVec *boneMatrices = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, boneMatricesLoc, skeleton.nBones * 3);

	Skeleton_Update(&skeleton, animation, boneMatrices, cTime);

	cTime += 0.01;
	
	if(cTime > 1)
		cTime = 0;

	Model_Draw(&model);
}

static void Exit(void){
	RiggedModel_Free(&model, &skeleton, &animation, nAnimations);
	Model_Free(&stageModel);
	shaderProgramFree(&skinnedShader.program);
	DVLB_Free(skinnedShader.dvlb);
	shaderProgramFree(&standardShader.program);
	DVLB_Free(standardShader.dvlb);
}

int main(){

	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	romfsInit();

	static C3D_RenderBuf rb;
	C3D_RenderBufInit(&rb, 240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	rb.clearColor = CLEAR_COLOR;
	C3D_RenderBufClear(&rb);
	C3D_RenderBufBind(&rb);

	Init();

	csndInit();

	while (aptMainLoop())
	{
		C3D_VideoSync();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break;

		Render();
		C3D_Flush();
		C3D_RenderBufTransfer(&rb, (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), DISPLAY_TRANSFER_FLAGS);
		C3D_RenderBufClear(&rb);

	}

	csndExit();
	romfsExit();
	Exit();
	C3D_Fini();
	gfxExit();
	return 0;
}
