#ifndef MOTAVIO_DEF
#define MOTAVIO_DEF

#include "model.h"
#include "characters.h"

typedef struct {

	Model model;
	Skeleton skeleton;
	Animation animations[1];
	PlayingAnimation playingAnims[1];

	Vec3 pos;
	Vec3 vel;

	float walkSpeed;

} Motavio;

void Motavio_Create(Character *character);

#endif