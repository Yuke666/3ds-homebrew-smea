#ifndef MOTAVIO_DEF
#define MOTAVIO_DEF

#include "characters.h"

typedef struct {

	Vec3 pos;
	Vec3 vel;

	float walkSpeed;

} Motavio;

void Motavio_Create(Character *character);

#endif