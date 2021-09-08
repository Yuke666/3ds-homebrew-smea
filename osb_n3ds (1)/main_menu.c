#include <stdio.h>
#include <sys/socket.h>
#include "main_menu.h"
#include "memory.h"
#include "client.h"
#include "text.h"
#include "utils.h"
#include "window.h"
#include "sound.h"
#include "images.h"
#include "sounds.h"

typedef struct {
	C3D_Tex 	mainMenuSprites;
	Sound		startSound;
	Rect2D 		startButtonRect;
	u8 			inQueue;
	float		queueTime;
	float		connected;
	float		started;
	char 		serverMessage[MAX_PACKET_SIZE];

} MainMenu;

static void Close(Game *game);
static void RenderBottom(Game *game);
static void RenderTop(Game *game);
static void Update(Game *game);

void MainMenu_Init(Game *game){

	game->data = Memory_StackAlloc(MAIN_STACK, sizeof(MainMenu));

	MainMenu *menu = (MainMenu *)game->data;

	menu->inQueue = 0;
	menu->connected = 0;
	menu->started = 0;

	strcpy(menu->serverMessage, "No response from server.");

	// C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_ALL);

	Sound_Load(&menu->startSound, SOUND_START);

	Utils_LoadImage(&menu->mainMenuSprites, IMAGE_MAIN_MENU_SPRITES, GPU_LINEAR, 4);

	game->initialStackPos = Memory_GetStack(MAIN_STACK);

    menu->startButtonRect = (Rect2D){0,0,menu->mainMenuSprites.width,menu->mainMenuSprites.height};
    menu->startButtonRect.x = (BOTTOM_WIDTH / 2) - (menu->mainMenuSprites.width / 2);
    menu->startButtonRect.y = (BOTTOM_HEIGHT / 2) - (menu->mainMenuSprites.height / 2);

	game->Update = Update;
	game->RenderBottom = RenderBottom;
	game->RenderTop = RenderTop;
	game->Close = Close;
}

static void Close(Game *game){

	MainMenu *menu = (MainMenu *)game->data;

	C3D_TexDelete(&menu->mainMenuSprites);

	Memory_SetStack(MAIN_STACK, game->initialStackPos);

	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
}

static void RenderBottom(Game *game){

	MainMenu *menu = (MainMenu *)game->data;
	
	float projView[16];

	Math_Ortho(projView, 0, BOTTOM_WIDTH, 0, BOTTOM_HEIGHT, 0.01f, 1000.0f);

	BindShader(&standard2DShader);
	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standard2DShader.projViewLoc, 4), projView);

	// if(!menu->started){

		DrawRect(menu->startButtonRect, (Rect2D){0,0,1,1}, &menu->mainMenuSprites);
	// }

}

static void RenderTop(Game *game){

	MainMenu *menu = (MainMenu *)game->data;

	float projView[16];

	Math_Ortho(projView, 0, TOP_WIDTH, 0, TOP_HEIGHT, 0.01f, 1000.0f);

	BindShader(&standard2DShader);
	Utils_SetUniformMatrix(C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, standard2DShader.projViewLoc, 4), projView);

	// float currTime = ((float)GetCurrTime()) / 1000.0f;

	int y = 6;
	
	// if(Client_IsConnected() == -1){

		char errMsg[CLIENT_ERR_MSG_LEN];

		Client_GetErrorMessage(errMsg);

		Text_Draw(FONT0, 6, y, 0, 2, TOP_WIDTH, errMsg);
		
		y += 14;

	// } else {

	// 	Text_Draw(FONT0, 6, y, 0, 2, TOP_WIDTH, "Connected to server");

	// 	y += 14;
	// }

	// if(menu->started){
	
		int len = Client_Recv(menu->serverMessage, MAX_PACKET_SIZE);

		if(len > 0)
			menu->serverMessage[len] = 0;

		Text_Draw(FONT0, 6, y, 0, 2, TOP_WIDTH, menu->serverMessage);

		y += 14;
	// }


	// if(menu->connected){

	// 	Text_Draw(FONT0, 6, y += 12, 0, 0, "Connected to server.");
	// }

	// if(menu->inQueue){

		// char buffer[32];
		// sprintf(buffer, "In queue for %.2f seconds.", currTime);

		// Text_Draw(FONT0, 6, y, 0, 0, TOP_WIDTH, buffer);

		// y += 12;
	// }
}


static void Update(Game *game){
	
	MainMenu *menu = (MainMenu *)game->data;

	if(!menu->started){

		touchPosition touch;	
		hidTouchRead(&touch);

		Rect2D sbRect = menu->startButtonRect;

		if(touch.py > sbRect.y && touch.py < sbRect.y + sbRect.h && 
			touch.px > sbRect.x && touch.px < sbRect.x + sbRect.w){

			Sound_Play(0x8, &menu->startSound);

			menu->started = 1;
		}
	}

	// menu->inQueue = 1;
	// menu->queueTime = GetCurrTime();
}