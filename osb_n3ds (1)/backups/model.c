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

int Model_Load(Model *model, const u8 *pointer){

	u8 *data = (u8 *)pointer;

	int flags, w, h;
	COPY_MEM_INC_FROM(&flags, &data, sizeof(int));
	COPY_MEM_INC_FROM(&w, &data, sizeof(int));
	COPY_MEM_INC_FROM(&h, &data, sizeof(int));

	C3D_TexInit(&model->tex, w, h, NUM_IMAGE_CHANNELS == 3 ? GPU_RGB8 : GPU_RGBA8);
	C3D_TexUpload(&model->tex, data);
	C3D_TexSetFilter(&model->tex, GPU_LINEAR, GPU_NEAREST);
	C3D_TexBind(0, &model->tex);

	data += w * h * NUM_IMAGE_CHANNELS;

	int nVerts;
	COPY_MEM_INC_FROM(&nVerts, &data, sizeof(int));

	int stride = sizeof(Vec3);
	int attribCount = 1;

	if(flags & 0x01){
		++attribCount;
		stride += sizeof(Vec2);
	}

	if(flags & (0x01 << 1)){
		++attribCount;
		stride += sizeof(Vec3);
	}

	if(flags & (0x01 << 2)){
		attribCount += 2;
		stride += sizeof(Vec4) * 2;
	}

	int size = stride * nVerts;

	model->vboData = (u8 *)linearAlloc(size);

	COPY_MEM_INC_FROM(model->vboData, &data, size);

	COPY_MEM_INC_FROM(&model->nElements, &data, sizeof(int));

	model->elements = (u16 *)linearAlloc(sizeof(u16) * model->nElements);

	COPY_MEM_INC_FROM(model->elements, &data, sizeof(u16) * model->nElements);

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, model->vboData, stride, attribCount, 0x43210);

	return data - pointer;
}

void Model_Free(Model *model){
	C3D_TexDelete(&model->tex);
	linearFree(model->vboData);
	linearFree(model->elements);
}

static int LoadAnimation(Animation *anim, const u8 *pointer){

	u8 *data = (u8 *)pointer;

	memset(anim, 0, sizeof(Animation));

	int nBones;
	COPY_MEM_INC_FROM(&nBones, &data, sizeof(int));
	
	int k;
	for(k = 0; k < nBones; k++){

		COPY_MEM_INC_FROM(&anim->nKeyframes[k], &data, sizeof(int));

		if(!anim->nKeyframes[k]) continue;

		anim->keyframes[k] = linearAlloc(sizeof(Keyframe) * anim->nKeyframes[k]);
		memset(anim->keyframes[k], 0, sizeof(Keyframe) * anim->nKeyframes[k]);

		int j;
		for(j = 0; j < anim->nKeyframes[k]; j++){

			COPY_MEM_INC_FROM(&anim->keyframes[k][j].frame, &data, sizeof(int));
			COPY_MEM_INC_FROM(&anim->keyframes[k][j].boneIndex, &data, sizeof(int));
			COPY_MEM_INC_FROM(&anim->keyframes[k][j].pos, &data, sizeof(Vec3));
			COPY_MEM_INC_FROM(&anim->keyframes[k][j].rot, &data, sizeof(Quat));

			if(anim->keyframes[k][j].frame > anim->length)
				anim->length = anim->keyframes[k][j].frame;

		}
	}

	return data - pointer;
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

int Yuk_LoadSkeleton(Skeleton *skeleton, const u8 *pointer){

	u8 *data = (u8 *)pointer;

	memset(skeleton, 0, sizeof(Skeleton));

	COPY_MEM_INC_FROM(&skeleton->nBones, &data, sizeof(int));

	int k;
	for(k = 0; k < skeleton->nBones; k++){

		int index;
		int parentIndex;

		COPY_MEM_INC_FROM(&parentIndex, &data, sizeof(int));
		COPY_MEM_INC_FROM(&index, &data, sizeof(int));

		Bone *bone = &skeleton->bones[index];

		bone->nChildren = 0;

		bone->index = index;

		COPY_MEM_INC_FROM(&bone->pos, &data, sizeof(Vec3));
		COPY_MEM_INC_FROM(&bone->rot, &data, sizeof(Quat));

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

	return data - pointer;
}

int RiggedModel_Load(Model *model, Skeleton *skeleton, Animation *animations, int *nAnimations, const u8 *pointer){

	u8 *data = (u8 *)pointer;

	data += Model_Load(model, data);

	data += Yuk_LoadSkeleton(skeleton, data);

	COPY_MEM_INC_FROM(nAnimations, &data, sizeof(int));

	int j;
	for(j = 0; j < *nAnimations; j++){

		data += LoadAnimation(&animations[j], data);
	}

	return data - pointer;
}

void RiggedModel_Free(Model *model, Skeleton *skeleton, Animation *animations, int nAnimations){

	Model_Free(model);

	int j;
	for(j = 0; j < nAnimations; j++)
		Yuk_FreeAnimation(animations[j]);

}