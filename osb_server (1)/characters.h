#ifndef CHARACTERS_DEF
#define CHARACTERS_DEF

#include "math.h"
#include "server.h"

#define MAX_CHARACTER_DATA_SIZE 1028

enum {
	CHARACTER_MOTAVIO = 0,
	NUM_CHARACTERS
};

typedef struct Character Character;

struct Character {

	u8 data[MAX_CHARACTER_DATA_SIZE];
	int index;

	void (*Update)(Character *character);
	void (*Packet)(Character *character, void *packet, int size);
};

void Character_Create(Character *character, int index);

#endif