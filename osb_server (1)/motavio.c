#include "motavio.h"
#include "stage.h"

enum {
	MOTAVIO_WALK_ANIM = 0,
};

static void Packet(Character *character, void *packet, int size){

}

static void Update(Character *character){

}

void Motavio_Create(Character *character){

	character->Packet = Packet;
	character->Update = Update;

	Motavio *motavio = (Motavio *)character->data;

	motavio->pos = (Vec3){0, 1.15, 0.1};

	motavio->walkSpeed = 0.35;
}