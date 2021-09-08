#ifndef CHARACTERS_DEF
#define CHARACTERS_DEF

#include <3ds.h>
#include "math.h"

enum {
	CHARACTER_MOTAVIO = 0,
	NUM_CHARACTERS
};

typedef struct Character Character;

struct Character {

	void *data;
	u8 index;

	Rect2D boundingBox;

	void (*Update)(Character *character);
	void (*Render)(Character *character);
	void (*Free)(Character *character);
	void (*Event)(Character *character);
	void (*Packet)(Character *character, void *packet, u16 size);

};

void Character_Create(Character *character, u8 index);

#endif