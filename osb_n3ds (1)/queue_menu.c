#include <stdio.h>
#include <string.h>
#include "queue_menu.h"
#include "sound.h"
#include "stage.h"
#include "sounds.h"
#include "memory.h"
#include "images.h"
#include "window.h"
#include "deflate.h"
#include "text.h"
#include "math.h"
#include "window.h"
#include "utils.h"

#define SPLASH_SUFFIX "_splash.img"
#define SPLASH_ART_FORMAT GPU_RGB8
#define PORTRAIT_SIZE 48
#define SPLASH_ART_WIDTH 96
#define SPLASH_ART_HEIGHT 128
#define PORTRAITS_X BOTTOM_WIDTH / PORTRAIT_SIZE

typedef struct {
	C3D_Tex 	portraits;
	C3D_Tex 	selectedSprtie;
	C3D_Tex 	splashTextures[4];
	u8 			nSelected;
	u8 			selectedCharacters[MAX_PLAYERS];
	u8 			playerIndex;
	Sound 		selectSound;
} QueueMenu;

static void Close(Game *game);
static void RenderBottom(Game *game);
static void RenderTop(Game *game);
static void Update(Game *game);

static void Select(QueueMenu *menu, int character){

	char path[16];

	sprintf(path, "%i%s", character, SPLASH_SUFFIX);

	FILE *fp = fopen(path, "rb");

	if(!fp) return;

	int w, h;
	fread(&w, 1, sizeof(int), fp);
	fread(&h, 1, sizeof(int), fp);

	C3D_Tex *tex = &menu->splashTextures[menu->nSelected];

	int nChannels = SPLASH_ART_FORMAT == GPU_RGBA8 ? 4 : 3;

	Deflate_Read(fp, tex->data, w * h * nChannels);

	fclose(fp);

	++menu->nSelected;
}

void QueueMenu_Init(Game *game){

	game->data = Memory_StackAlloc(MAIN_STACK, sizeof(QueueMenu));

	QueueMenu *menu = (QueueMenu *)game->data;

	// init

	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_ALL);

	game->initialStackPos = Memory_GetStack(MAIN_STACK);

    menu->nSelected = 0;
    menu->playerIndex = 0;

	// select
	
	Sound_Load(&menu->selectSound, SOUND_SELECT);

	Utils_LoadImage(&menu->selectedSprtie, IMAGE_MENU_SELECTED, GPU_LINEAR, 4);

    // portraits

	Utils_LoadImage(&menu->portraits, IMAGE_PORTRAITS, GPU_LINEAR, 3);

    // init splash textures

    int k;
	for(k = 0; k < 4; k++){

		C3D_TexInit(&menu->splashTextures[k], SPLASH_ART_WIDTH, SPLASH_ART_HEIGHT, SPLASH_ART_FORMAT);
		C3D_TexSetFilter(&menu->splashTextures[k], GPU_NEAREST, GPU_NEAREST);
		C3D_TexSetWrap(&menu->splashTextures[k], GPU_REPEAT, GPU_REPEAT);
	}

	game->Update = Update;
	game->RenderBottom = RenderBottom;
	game->RenderTop = RenderTop;
	game->Close = Close;
}

static void Close(Game *game){

	QueueMenu *menu = (QueueMenu *)game->data;

	C3D_TexDelete(&menu->portraits);

	int k;
	for(k = 0; k < 4; k++)
		C3D_TexDelete(&menu->splashTextures[k]);

	Memory_SetStack(MAIN_STACK, game->initialStackPos);

	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
}

static void RenderBottom(Game *game){

	QueueMenu *menu = (QueueMenu *)game->data;

	float projView[16];

	Math_Ortho(projView, 0, BOTTOM_WIDTH, 0, BOTTOM_HEIGHT, 0.01f, 1000.0f);

	BindShader(&standard2DShader);
	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standard2DShader.projViewLoc, 4), projView);

    Rect2D sRect = (Rect2D){0,0,menu->portraits.width,menu->portraits.height};
	DrawRect(sRect, (Rect2D){0,0,1,1}, &menu->portraits);


    int k;
    for(k = 0; k < menu->nSelected; k++){

		Rect2D sRect = (Rect2D){0,0,PORTRAIT_SIZE,PORTRAIT_SIZE};

		sRect.x = (menu->selectedCharacters[k] - 1) % PORTRAITS_X;
		sRect.y = (menu->selectedCharacters[k] - 1) / PORTRAITS_X;


		DrawRect(sRect, (Rect2D){0,0,1,1}, &menu->selectedSprtie);
    }
}

static void RenderTop(Game *game){

	QueueMenu *menu = (QueueMenu *)game->data;

	float projView[16];

	Math_Ortho(projView, 0, TOP_WIDTH, 0, TOP_HEIGHT, 0.01f, 1000.0f);

	BindShader(&standard2DShader);
	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standard2DShader.projViewLoc, 4), projView);

	int k;
	for(k = 0; k < menu->nSelected; k++){
	    Rect2D screenRect = (Rect2D){(SPLASH_ART_WIDTH) * k,0,SPLASH_ART_WIDTH,SPLASH_ART_HEIGHT};
		DrawRect(screenRect, (Rect2D){0,0,1,1}, &menu->splashTextures[k]);
	}


	touchPosition touch;	

	hidTouchRead(&touch);

	char buffer[8];

	sprintf(buffer, "%i %i", touch.px, touch.py);

	Text_Draw(FONT0, 32, 32, 0, 0, TOP_WIDTH, buffer);
}


static void Update(Game *game){
	
	QueueMenu *menu = (QueueMenu *)game->data;


	touchPosition touch;	
	hidTouchRead(&touch);

	if(touch.px != 0 || touch.py != 0){

		int x = floor(touch.px / (float)PORTRAIT_SIZE);
		int y = floor(touch.py / (float)PORTRAIT_SIZE);

		int index = ((y * PORTRAITS_X) + x) + 1;

		menu->selectedCharacters[menu->playerIndex] = index;		
		
		Select(menu, index-1);

		Sound_Play(0x8, &menu->selectSound);
	}
}