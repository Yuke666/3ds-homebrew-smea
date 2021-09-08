#include <stdio.h>
#include <string.h>
#include "menu.h"
#include "deflate.h"
#include "text.h"
#include "math.h"
#include "window.h"
#include "utils.h"

#define MAX_CHARACTERS_X 8
#define TOP_WIDTH 400
#define TOP_HEIGHT 240
#define BOTTOM_WIDTH 320
#define BOTTOM_HEIGHT 240
#define CHARACTERS_START_X 32
#define CHARACTERS_START_Y 32
#define NUM_CHARACTERS 2
#define PORTRAIT_SIZE 32
#define PORTRAIT_PADDING 4
#define SPLASH_ART_WIDTH 128
#define SPLASH_ART_HEIGHT 240

static float projView[16];
static Vertex22 bottomBgData[6];

static void Select(CharacterMenu *menu, int character){

	char path[16];

	sprintf(path, "%i_splash.img", character);

	FILE *fp = fopen(path, "rb");

	if(!fp) return;

	int w, h;
	fread(&w, 1, sizeof(int), fp);
	fread(&h, 1, sizeof(int), fp);

	C3D_Tex *tex = &menu->splashTextures[menu->nSelected].tex;

	C3D_TexInit(tex, w, h, GPU_RGBA8);

	Deflate_Read(fp, tex->data, w * h * 4);

	fclose(fp);

	++menu->nSelected;
}

void CharacterMenu_Init(CharacterMenu *menu){

	Sound_Load(&menu->selectSound, "select.snd");

	Utils_LoadImage(&menu->bottomBG.tex, "menu_bg.img", GPU_LINEAR, 3);
	Utils_LoadImage(&menu->topBG.tex, "menu_bg_top.img", GPU_LINEAR, 3);
	Utils_LoadImage(&menu->portraits.tex, "portraits.img", GPU_LINEAR, 3);

	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_ALL);

    menu->bgScroll = 0;
    menu->nSelected = 0;


	// bottom background

	Utils_Vertex22Rect(bottomBgData, (Rect2D){0,0,BOTTOM_WIDTH * 2,BOTTOM_HEIGHT}, (Rect2D){0,0,1,1});

    menu->bottomBG.vboData = (u8 *)linearAlloc(sizeof(bottomBgData));
    memcpy(menu->bottomBG.vboData, bottomBgData, sizeof(bottomBgData));

    BufInfo_Init(&menu->bottomBG.bufInfo);
    BufInfo_Add(&menu->bottomBG.bufInfo, menu->bottomBG.vboData, sizeof(Vertex22), 2, 0x10);

    // top background

    menu->topBG.vboData = (u8 *)linearAlloc(6 * sizeof(Vertex22));
	Utils_Vertex22Rect((Vertex22 *)menu->topBG.vboData, (Rect2D){0,0,TOP_WIDTH,TOP_HEIGHT}, (Rect2D){0,0,1,1});

    BufInfo_Init(&menu->topBG.bufInfo);
    BufInfo_Add(&menu->topBG.bufInfo, menu->topBG.vboData, sizeof(Vertex22), 2, 0x10);

    // portratis

    menu->portraits.vboData = (u8 *)linearAlloc(6 * NUM_CHARACTERS * sizeof(Vertex22));

	Rect2D img = (Rect2D){0,0,PORTRAIT_SIZE/(float)menu->portraits.tex.width,PORTRAIT_SIZE/(float)menu->portraits.tex.height};

    int k;
    for(k = 0; k < NUM_CHARACTERS; k++){

    	Rect2D screen = (Rect2D){0,0,PORTRAIT_SIZE,PORTRAIT_SIZE};
    	
    	int x = k % MAX_CHARACTERS_X;
    	int y = k / MAX_CHARACTERS_X;

    	screen.x = CHARACTERS_START_X + (x * (PORTRAIT_SIZE + PORTRAIT_PADDING));
    	screen.y = CHARACTERS_START_Y + (y * (PORTRAIT_SIZE + PORTRAIT_PADDING));

    	x = k % (menu->portraits.tex.width/PORTRAIT_SIZE);
    	y = k / (menu->portraits.tex.width/PORTRAIT_SIZE);

    	img.x = (x * img.w);
    	img.y = (y * img.h);

		Utils_Vertex22Rect(&(((Vertex22 *)menu->portraits.vboData)[k * 6]), screen, img);
    }

    BufInfo_Init(&menu->portraits.bufInfo);
    BufInfo_Add(&menu->portraits.bufInfo, menu->portraits.vboData, sizeof(Vertex22), 2, 0x10);


    // init splash textures

	for(k = 0; k < 4; k++){

		C3D_TexInit(&menu->splashTextures[k].tex, SPLASH_ART_WIDTH, SPLASH_ART_HEIGHT, GPU_RGBA8);
		C3D_TexSetFilter(&menu->splashTextures[k].tex, GPU_NEAREST, GPU_NEAREST);
		C3D_TexSetWrap(&menu->splashTextures[k].tex, GPU_REPEAT, GPU_REPEAT);
	
	    menu->splashTextures[k].vboData = (u8 *)linearAlloc(6 * sizeof(Vertex22));

	    Rect2D screenRect = (Rect2D){(SPLASH_ART_WIDTH) * k,0,SPLASH_ART_WIDTH,SPLASH_ART_HEIGHT};

		Utils_Vertex22Rect((Vertex22 *)menu->splashTextures[k].vboData, screenRect, (Rect2D){0,0,1,1});

	    BufInfo_Init(&menu->splashTextures[k].bufInfo);
	    BufInfo_Add(&menu->splashTextures[k].bufInfo, menu->splashTextures[k].vboData, sizeof(Vertex22), 2, 0x10);
	}

	Select(menu, 0);
}

void CharacterMenu_Close(CharacterMenu *menu){

	Sound_Close(&menu->selectSound);
	C3D_TexDelete(&menu->bottomBG.tex);
	C3D_TexDelete(&menu->topBG.tex);
	C3D_TexDelete(&menu->portraits.tex);

	int k;
	for(k = 0; k < 4; k++)
		C3D_TexDelete(&menu->splashTextures[k].tex);

	linearFree(menu->bottomBG.vboData);
	linearFree(menu->topBG.vboData);
}

void CharacterMenu_RenderBottom(CharacterMenu *menu){

	Math_Ortho(projView, 0, BOTTOM_WIDTH, 0, BOTTOM_HEIGHT, 0.01f, 1000.0f);

	C3D_BindProgram(&standard2DShader.program);
	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standard2DShader.projViewLoc, 4), projView);

	menu->bgScroll += 1.0f;

	if(menu->bgScroll > BOTTOM_WIDTH)
		menu->bgScroll = 0;

	int k;
	for(k = 0; k < 6; k++){
		((Vertex22 *)menu->bottomBG.vboData)[k].pos.x = bottomBgData[k].pos.x - menu->bgScroll;
	}

    C3D_SetBufInfo(&menu->bottomBG.bufInfo);
    C3D_TexBind(0, &menu->bottomBG.tex);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 6);

    C3D_SetBufInfo(&menu->portraits.bufInfo);
    C3D_TexBind(0, &menu->portraits.tex);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 6 * NUM_CHARACTERS);

}

void CharacterMenu_RenderTop(CharacterMenu *menu){

	Math_Ortho(projView, 0, TOP_WIDTH, 0, TOP_HEIGHT, 0.01f, 1000.0f);

	C3D_BindProgram(&standard2DShader.program);
	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standard2DShader.projViewLoc, 4), projView);

    C3D_SetBufInfo(&menu->topBG.bufInfo);
    C3D_TexBind(0, &menu->topBG.tex);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 6);

    C3D_SetBufInfo(&menu->splashTextures[0].bufInfo);
    C3D_TexBind(0, &menu->splashTextures[0].tex);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 6);

	touchPosition touch;	

	hidTouchRead(&touch);

	char buffer[8];

	sprintf(buffer, "%i %i", touch.px, touch.py);

	Text_Draw(FONT0, 32, 32, 0, 0, buffer);
}


void CharacterMenu_Update(CharacterMenu *menu){
	
	touchPosition touch;	
	hidTouchRead(&touch);

	if(touch.px != 0 || touch.py != 0){

		Sound_Play(0x8, &menu->selectSound);
	}
}