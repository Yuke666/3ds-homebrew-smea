#include "characters.h"
#include "motavio.h"

void (*CreateFuncs[])(Character *character) = {

	Motavio_Create,
};


void Character_Create(Character *character, u8 index){

	CreateFuncs[index](character);
}