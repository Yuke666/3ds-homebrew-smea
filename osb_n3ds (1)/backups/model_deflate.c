#include <stdio.h>
#include "deflate.h"
#include "model.h"
#include "math.h"

#define NUM_IMAGE_CHANNELS 3

#define COPY_MEM_INC_FROM(into, from, size) do { memcpy(into, *from, size); *from += size; } while(0);

static void BoneUpdate(Bone *bone, Animation anim, float cTime, C3D_FVec *matrices){

	Quat rot = bone->rot;

	Keyframe *prev = NULL;
	Keyframe *next = NULL;

	int f;
	for(f = 0; f < anim.nKeyframes[bone->index]; f++){

		if(!next && anim.keyframes[bone->index][f].frame * ANIMATION_FRAME_RATE > cTime)
			next = &anim.keyframes[bone->index][f];

		if(anim.keyframes[bone->index][f].frame * ANIMATION_FRAME_RATE <= cTime)
			prev = &anim.keyframes[bone->index][f];
	}

	if(!prev){
		if(anim.nKeyframes[bone->index])
			prev = &anim.keyframes[bone->index][anim.nKeyframes[bone->index]-1];
	}

	if(prev && next){

		float animTime = (next->frame - prev->frame) * ANIMATION_FRAME_RATE;
		float t = (cTime - (prev->frame*ANIMATION_FRAME_RATE)) / animTime;

		if(t > 1) t = 1;
		if(t < 0) t = 0;

		rot = Math_QuatMult(bone->rot, Math_Slerp(prev->rot, next->rot, t));
	}

	else if(!next && prev)
		rot = Math_QuatMult(bone->rot, prev->rot);

	static float matrix[16];
	Math_TranslateMatrix(matrix, bone->pos);
	Math_MatrixFromQuat(rot, bone->absMatrix);
	Math_MatrixMatrixMult(bone->absMatrix, matrix, bone->absMatrix);

	if(bone->parent)
		Math_MatrixMatrixMult(bone->absMatrix, bone->parent->absMatrix, bone->absMatrix); 

	Math_MatrixMatrixMult(matrix, bone->absMatrix, bone->invBindMatrix);

	matrices[(bone->index*3)].w = matrix[3];
	matrices[(bone->index*3)].x = matrix[0];
	matrices[(bone->index*3)].y = matrix[1];
	matrices[(bone->index*3)].z = matrix[2];

	matrices[(bone->index*3)+1].w = matrix[7];
	matrices[(bone->index*3)+1].x = matrix[4];
	matrices[(bone->index*3)+1].y = matrix[5];
	matrices[(bone->index*3)+1].z = matrix[6];

	matrices[(bone->index*3)+2].w = matrix[11];
	matrices[(bone->index*3)+2].x = matrix[8];
	matrices[(bone->index*3)+2].y = matrix[9];
	matrices[(bone->index*3)+2].z = matrix[10];

    int j;
	for(j = 0; j < bone->nChildren; j++)
		BoneUpdate(bone->children[j], anim, cTime, matrices);
}

void Skeleton_Update(Skeleton *skeleton, Animation anim, C3D_FVec *matrices, float cTime){

	BoneUpdate(skeleton->rootBone, anim, cTime, matrices);
}

static void LoadModel(Model *model, FILE *fp, u8 stride, u8 attribCount, u64 permutation){

	fread(&model->nTextures, 1, sizeof(int), fp);
		
	int k;
	for(k = 0; k < model->nTextures; k++){
	
		int w, h;

		fread(&w, 1, sizeof(int), fp);
		fread(&h, 1, sizeof(int), fp);

		C3D_TexInit(&model->textures[k], w, h, NUM_IMAGE_CHANNELS == 3 ? GPU_RGB8 : GPU_RGBA8);
		Deflate_Read(fp, model->textures[k].data, w * h * NUM_IMAGE_CHANNELS);
		// C3D_TexUpload(&model->textures[k], data); | not needed, all it does is copy into it's data anyway.
		C3D_TexSetFilter(&model->textures[k], GPU_LINEAR, GPU_NEAREST);
		C3D_TexSetWrap(&model->textures[k], GPU_REPEAT, GPU_REPEAT);
	}

	int nVerts;
	fread(&nVerts, 1, sizeof(int), fp);

	int size = stride * nVerts;

	model->vboData = (u8 *)linearAlloc(size);

	Deflate_Read(fp, model->vboData, size);

	for(k = 0; k < model->nTextures; k++){

		fread(&model->nElements[k], 1, sizeof(int), fp);

		model->elements[k] = (u16 *)linearAlloc(sizeof(u16) * model->nElements[k]);

		fread(model->elements[k], model->nElements[k], sizeof(u16), fp);

		// Deflate_Read(fp, model->elements[k], sizeof(u16) * model->nElements[k]);
	}

	C3D_BufInfo *bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, model->vboData, stride, attribCount, permutation);
}

void Model_Load(Model *model, const char *path){

	FILE *fp = fopen(path, "rb");

	LoadModel(model, fp, sizeof(Vec3) + sizeof(Vec2), 2, 0x10);

	fclose(fp);
}

void Model_Free(Model *model){
	
	int k;
	for(k = 0; k < model->nTextures; k++){

		C3D_TexDelete(&model->textures[k]);
		linearFree(model->elements[k]);
	}

	linearFree(model->vboData);
}

static void LoadAnimation(Animation *anim, FILE *fp){

	memset(anim, 0, sizeof(Animation));

	int nBones;
	fread(&nBones, 1, sizeof(int), fp);
	
	int k;
	for(k = 0; k < nBones; k++){

		fread(&anim->nKeyframes[k], 1, sizeof(int), fp);

		if(!anim->nKeyframes[k]) continue;

		anim->keyframes[k] = linearAlloc(sizeof(Keyframe) * anim->nKeyframes[k]);
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
}

void Yuk_FreeAnimation(Animation anim){

	int k;
	for(k = 0; k < MAX_BONES; k++)
		if(anim.keyframes[k])
			linearFree(anim.keyframes[k]);

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

void Yuk_LoadSkeleton(Skeleton *skeleton, FILE *fp){

	memset(skeleton, 0, sizeof(Skeleton));

	fread(&skeleton->nBones, 1, sizeof(int), fp);

	int k;
	for(k = 0; k < skeleton->nBones; k++){

		int index;
		int parentIndex;

		fread(&parentIndex, 1, sizeof(int), fp);
		fread(&index, 1, sizeof(int), fp);

		Bone *bone = &skeleton->bones[index];

		bone->nChildren = 0;

		bone->index = index;

		fread(&bone->pos, 1, sizeof(Vec3), fp);
		fread(&bone->rot, 1, sizeof(Quat), fp);

		bone->parent = NULL;

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

void RiggedModel_Load(Model *model, Skeleton *skeleton, Animation *animations, int *nAnimations, const char *path){

	FILE *fp = fopen(path, "rb");

	LoadModel(model, fp, sizeof(Vec3) + sizeof(Vec2) + (sizeof(Vec4) * 2), 4, 0x3210);

	Yuk_LoadSkeleton(skeleton, fp);

	fread(nAnimations, 1, sizeof(int), fp);

	int j;
	for(j = 0; j < *nAnimations; j++){

		LoadAnimation(&animations[j], fp);
	}

	fclose(fp);
}

void RiggedModel_Free(Model *model, Skeleton *skeleton, Animation *animations, int nAnimations){

	Model_Free(model);

	int j;
	for(j = 0; j < nAnimations; j++)
		Yuk_FreeAnimation(animations[j]);

}

void Model_Draw(Model *model){

	int k;
	for(k = 0; k < model->nTextures; k++){

		C3D_TexBind(0, &model->textures[k]);

		C3D_DrawElements(GPU_TRIANGLES, model->nElements[k], C3D_UNSIGNED_SHORT, (void *)model->elements[k]);
	}
}