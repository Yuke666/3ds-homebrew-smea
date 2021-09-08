#ifndef MODEL_DEF
#define MODEL_DEF

#include <3ds.h>
#include <citro3d.h>
#include "math.h"
#include "memory.h"

#define MAX_MODEL_TEXTURES 16
#define MAX_BONES 18
#define BONE_MAX_CHILDREN 3
#define ANIMATION_FRAME_RATE 60

typedef struct Bone Bone;

struct Bone {
	u8 index;
	u8 nChildren;
	Bone *parent;
	Vec3 pos;
	Quat rot;
	Bone *children[BONE_MAX_CHILDREN];
	float absMatrix[16];
	float invBindMatrix[16];
};

typedef struct {
	int frame;
	u8 boneIndex;
	Vec3 pos;
	Quat rot;
} Keyframe;

typedef struct {
	Keyframe *keyframes[MAX_BONES];
	u8 nKeyframes[MAX_BONES];
	u8 length;
} Animation;

typedef struct {
	Animation *anim;
	float weight;
	float into;
} PlayingAnimation;

typedef struct {
	Bone bones[MAX_BONES];
	Bone *rootBone;
	u8 nBones;
} Skeleton;

typedef struct {
	u8 *vboData;
	u8 nTextures;
	u16 nElements[MAX_MODEL_TEXTURES];
	u16 *elements[MAX_MODEL_TEXTURES];
	C3D_Tex textures[MAX_MODEL_TEXTURES];
	C3D_BufInfo bufInfo;
} Model;

void Model_Free(Model *model);
void RiggedModel_Free(Model *model, Skeleton *skeleton, Animation *animations, int nAnimations);
int Model_Load(Model *model, const char *path);
int RiggedModel_Load(Model *model, Skeleton *skeleton, Animation *animations, const char *path);
void Skeleton_Update(Skeleton *skeleton, PlayingAnimation *anims, int nAnims, C3D_FVec *matrices);
void Model_Draw(Model *model);
void Model_DeleteTextures(Model *model);

#endif