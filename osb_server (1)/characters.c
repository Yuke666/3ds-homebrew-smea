#include "characters.h"
#include "motavio.h"

void (*CreateFuncs[])(Character *character) = {

	Motavio_Create,
};


void Character_Create(Character *character, int index){

	CreateFuncs[index](character);
}