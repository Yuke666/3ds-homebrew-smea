#include <stdio.h>
#include "deflate.h"
#include "model.h"
#include "math.h"

#define NUM_IMAGE_CHANNELS 4

#define COPY_MEM_INC_FROM(into, from, size) do { memcpy(into, *from, size); *from += size; } while(0);

static Quat GetAnimsBoneRot(Bone *bone, PlayingAnimation *pAnim){

	Animation *anim = pAnim->anim;

	Keyframe *prev = NULL;
	Keyframe *next = NULL;

	int f;
	for(f = 0; f < anim->nKeyframes[bone->index]; f++){

		if(anim->keyframes[bone->index][f].frame <= pAnim->into){

			prev = &anim->keyframes[bone->index][f];

		} else {

			next = &anim->keyframes[bone->index][f];
			break;
		}
	}

	if(!next)
		return prev->rot;

	float slerp = (pAnim->into - prev->frame) / (next->frame - prev->frame);

	return Math_Slerp(prev->rot, next->rot, slerp);
}

static void BoneUpdate(Bone *bone, PlayingAnimation *anims, int nAnims, C3D_FVec *matrices){

	Quat rot = (Quat){0,0,0,1};

	int j;
	for(j = 0; j < nAnims; j++)
		if(anims[j].anim->nKeyframes[bone->index])
			rot = Math_Slerp(rot, GetAnimsBoneRot(bone, &anims[j]), anims[j].weight);

	rot = Math_QuatMult(bone->rot, rot);

	static float matrix[16];
	Math_TranslateMatrix(matrix, bone->pos);
	Math_MatrixFromQuat(rot, bone->absMatrix);
	Math_MatrixMatrixMult(bone->absMatrix, matrix, bone->absMatrix);

	if(bone->parent)
		Math_MatrixMatrixMult(bone->absMatrix, bone->parent->absMatrix, bone->absMatrix); 

	Math_MatrixMatrixMult(matrix, bone->absMatrix, bone->invBindMatrix);

	matrices[(bone->index*3)].x = matrix[0];
	matrices[(bone->index*3)].y = matrix[1];
	matrices[(bone->index*3)].z = matrix[2];
	matrices[(bone->index*3)].w = matrix[3];

	matrices[(bone->index*3)+1].x = matrix[4];
	matrices[(bone->index*3)+1].y = matrix[5];
	matrices[(bone->index*3)+1].z = matrix[6];
	matrices[(bone->index*3)+1].w = matrix[7];

	matrices[(bone->index*3)+2].x = matrix[8];
	matrices[(bone->index*3)+2].y = matrix[9];
	matrices[(bone->index*3)+2].z = matrix[10];
	matrices[(bone->index*3)+2].w = matrix[11];

	for(j = 0; j < bone->nChildren; j++)
		BoneUpdate(bone->children[j], anims, nAnims, matrices);
}

void Skeleton_Update(Skeleton *skeleton, PlayingAnimation *anims, int nAnims, C3D_FVec *matrices){

	BoneUpdate(skeleton->rootBone, anims, nAnims, matrices);

}

static int LoadModel(Model *model, FILE *fp, u8 stride, u8 attribCount, u64 permutation){

	fread(&model->nTextures, 1, sizeof(int), fp);
		
	int k;
	for(k = 0; k < model->nTextures; k++){
	
		int w, h;
		fread(&w, 1, sizeof(int), fp);
		fread(&h, 1, sizeof(int), fp);

		C3D_TexInit(&model->textures[k], w, h, NUM_IMAGE_CHANNELS == 3 ? GPU_RGB8 : GPU_RGBA8);
		Deflate_Read(fp, model->textures[k].data, w * h * NUM_IMAGE_CHANNELS);
		C3D_TexSetFilter(&model->textures[k], GPU_LINEAR, GPU_NEAREST);
		C3D_TexSetWrap(&model->textures[k], GPU_REPEAT, GPU_REPEAT);
	}

	int nVerts;
	fread(&nVerts, 1, sizeof(int), fp);

	int size = stride * nVerts;

	model->vboData = (u8 *)Memory_StackAlloc(MAIN_STACK, size);

	// Deflate_Read(fp, model->vboData, size);
	fread(model->vboData, 1, size, fp);

	for(k = 0; k < model->nTextures; k++){

		fread(&model->nElements[k], 1, sizeof(int), fp);

		model->elements[k] = (u16 *)Memory_StackAlloc(MAIN_STACK, sizeof(u16) * model->nElements[k]);

		fread(model->elements[k], model->nElements[k], sizeof(u16), fp);

		// Deflate_Read(fp, model->elements[k], sizeof(u16) * model->nElements[k]);
	}

	BufInfo_Init(&model->bufInfo);
	BufInfo_Add(&model->bufInfo, model->vboData, stride, attribCount, permutation);

	// nTextures elements + 1 vbo data
	return model->nTextures + 1;
}

int Model_Load(Model *model, const char *path){

	FILE *fp = fopen(path, "rb");

	int nAllocations = LoadModel(model, fp, sizeof(Vec3) + sizeof(Vec2), 2, 0x10);

	fclose(fp);

	return nAllocations;
}

void Model_DeleteTextures(Model *model){
	
	int k;
	for(k = 0; k < model->nTextures; k++)
		C3D_TexDelete(&model->textures[k]);
}

void Model_Free(Model *model){

	Model_DeleteTextures(model);

	// Frees the elements and the vbo data
	Memory_StackPop(MAIN_STACK, model->nTextures + 1);
}

static int LoadAnimation(Animation *anim, FILE *fp){

	int nBones;
	fread(&nBones, 1, sizeof(int), fp);
	
	int nAllocations = 0;

	int k;
	for(k = 0; k < nBones; k++){

		fread(&anim->nKeyframes[k], 1, sizeof(int), fp);

		if(!anim->nKeyframes[k]) continue;

		++nAllocations;

		anim->keyframes[k] = Memory_StackAlloc(MAIN_STACK, sizeof(Keyframe) * anim->nKeyframes[k]);

		memset(anim->keyframes[k], 0, sizeof(Keyframe) * anim->nKeyframes[k]);

		int j;
		for(j = 0; j < anim->nKeyframes[k]; j++){

			fread(&anim->keyframes[k][j].frame, 1, sizeof(int), fp);
			fread(&anim->keyframes[k][j].boneIndex, 1, sizeof(int), fp);
			fread(&anim->keyframes[k][j].pos, 1, sizeof(Vec3), fp);
			fread(&anim->keyframes[k][j].rot, 1, sizeof(Quat), fp);

			if(anim->keyframes[k][j].frame > anim->length)
				anim->length = anim->keyframes[k][j].frame;

		}
	}

	return nAllocations;
}

void FreeAnimation(Animation anim){

	int nAllocations = 0;

	int k;
	for(k = 0; k < MAX_BONES; k++)
		if(anim.keyframes[k])
			++nAllocations;

	Memory_StackPop(MAIN_STACK, nAllocations);
}

static void InitBone(Bone *bone){

	static float matrix[16];
	Math_TranslateMatrix(matrix, bone->pos);
	Math_MatrixFromQuat(bone->rot, bone->absMatrix);
	Math_MatrixMatrixMult(bone->absMatrix, matrix, bone->absMatrix);

	if(bone->parent)
		Math_MatrixMatrixMult(bone->absMatrix, bone->parent->absMatrix, bone->absMatrix); 

	memcpy(bone->invBindMatrix, bone->absMatrix, sizeof(float) * 16);
	Math_InverseMatrix(bone->invBindMatrix);

	int k;
	for(k = 0; k < bone->nChildren; k++)
		InitBone(bone->children[k]);
}

void LoadSkeleton(Skeleton *skeleton, FILE *fp){

	memset(skeleton, 0, sizeof(Skeleton));

	fread(&skeleton->nBones, 1, sizeof(int), fp);

	int k;
	for(k = 0; k < skeleton->nBones; k++){

		int index;
		int parentIndex;

		fread(&parentIndex, 1, sizeof(int), fp);
		fread(&index, 1, sizeof(int), fp);

		Bone *bone = &skeleton->bones[index];

		bone->index = index;

		fread(&bone->pos, 1, sizeof(Vec3), fp);
		fread(&bone->rot, 1, sizeof(Quat), fp);

		if(parentIndex >= 0){

			bone->parent = &skeleton->bones[parentIndex];

			if(bone->parent->nChildren < BONE_MAX_CHILDREN)
				bone->parent->children[bone->parent->nChildren++] = bone;
		
		} else {

			skeleton->rootBone = bone;
		}
	}

	InitBone(skeleton->rootBone);
}

int RiggedModel_Load(Model *model, Skeleton *skeleton, Animation *animations, const char *path){

	FILE *fp = fopen(path, "rb");

	int nAllocated = LoadModel(model, fp, sizeof(Vec3) + sizeof(Vec2) + (sizeof(Vec3) * 2), 4, 0x3210);

	LoadSkeleton(skeleton, fp);

	int nAnimations;

	fread(&nAnimations, 1, sizeof(int), fp);

	int j;
	for(j = 0; j < nAnimations; j++){

		nAllocated += LoadAnimation(&animations[j], fp);
	}

	fclose(fp);

	return nAllocated;
}

void RiggedModel_Free(Model *model, Skeleton *skeleton, Animation *animations, int nAnimations){

	Model_Free(model);

	int j;
	for(j = 0; j < nAnimations; j++)
		FreeAnimation(animations[j]);
}

void Model_Draw(Model *model){

	C3D_SetBufInfo(&model->bufInfo);

	int k;
	for(k = 0; k < model->nTextures; k++){

		C3D_TexBind(0, &model->textures[k]);
		C3D_DrawElements(GPU_TRIANGLES, model->nElements[k], C3D_UNSIGNED_SHORT, (void *)model->elements[k]);
	}
}