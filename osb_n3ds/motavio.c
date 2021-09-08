#include "motavio.h"
#include "utils.h"
#include "window.h"
#include "stage.h"
#include "memory.h"

#define MOTAVIO_N3DS_PATH "motavio.n3ds"
#define MOTAVIO_NUM_ANIMATIONS 1

enum {
	MOTAVIO_WALK_ANIM = 0,
};

static void Event(Character *character){

}

static void Packet(Character *character, void *packet, u16 size){

}

static void Update(Character *character){

}

static void Render(Character *character){

	Motavio *motavio = (Motavio *)character->data;	

	Stage_DrawShadow(motavio->pos, (Vec2){0.3, 0.3});

	float characterMatrix[16];
	Math_TranslateMatrix(characterMatrix, motavio->pos);

	BindShader(&skinnedShader);

	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, skinnedShader.modelLoc, 4), characterMatrix);

	C3D_FVec *boneMatrices = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, skinnedShader.bonesLoc, motavio->skeleton.nBones * 3);

	Skeleton_Update(&motavio->skeleton, motavio->playingAnims, 1, boneMatrices);

	PlayingAnimation *walkAnim = &motavio->playingAnims[MOTAVIO_WALK_ANIM];

	walkAnim->into += (float)GetDeltaTime() * (ANIMATION_FRAME_RATE * motavio->walkSpeed);

	walkAnim->weight = motavio->walkSpeed;
	
	if(walkAnim->into > motavio->animations[0].length)
		walkAnim->into = 0;

	Model_Draw(&motavio->model);
}

static void Free(Character *character){

	Motavio *motavio = (Motavio *)character->data;	
	
	RiggedModel_Free(&motavio->model, &motavio->skeleton, motavio->animations, MOTAVIO_NUM_ANIMATIONS);

	Memory_StackPop(MAIN_STACK, 1);
}

void Motavio_Create(Character *character){

	character->Packet = Packet;
	character->Update = Update;
	character->Event = Event;
	character->Render = Render;
	character->Free = Free;

	character->data = Memory_StackAlloc(MAIN_STACK, sizeof(Motavio));

	Motavio *motavio = (Motavio *)character->data;

	motavio->pos = (Vec3){0, 1.15, 0.1};

	character->boundingBox = (Rect2D){-0.25, 0.1, 0.4, 0.5};
	character->boundingBox.x += motavio->pos.x;
	character->boundingBox.y += motavio->pos.y;

	RiggedModel_Load(&motavio->model,&motavio->skeleton, motavio->animations, MOTAVIO_N3DS_PATH);

	PlayingAnimation *walkAnim = &motavio->playingAnims[MOTAVIO_WALK_ANIM];

	walkAnim->anim = &motavio->animations[0];	

	motavio->walkSpeed = 1;
}