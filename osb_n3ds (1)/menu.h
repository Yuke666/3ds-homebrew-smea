#ifndef MENU_DEF
#define MENU_DEF

#include <3ds.h>
#include <citro3d.h>
#include "sound.h"

typedef struct {

	struct {
		C3D_Tex tex;
		u8 *vboData;
		C3D_BufInfo bufInfo;
	} portraits;

	struct {
		C3D_Tex tex;
		u8 *vboData;
		C3D_BufInfo bufInfo;
	} bottomBG, topBG;

	struct {
		u8 *vboData;
		C3D_BufInfo bufInfo;
		C3D_Tex tex;
	} splashTextures[4];

	u8 nSelected;

	float bgScroll;
	Sound selectSound;

} CharacterMenu;

void CharacterMenu_Init(CharacterMenu *menu);
void CharacterMenu_Close(CharacterMenu *menu);
void CharacterMenu_RenderTop(CharacterMenu *menu);
void CharacterMenu_RenderBottom(CharacterMenu *menu);
void CharacterMenu_Update(CharacterMenu *menu);


#endif