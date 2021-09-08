#include "stage.h"
#include "characters.h"

#define MAX_STAGE_PLATFORMS 6

enum {
	PLATFORM_TYPE_BOTH = 0,
	PLATFORM_TYPE_ONE_WAY,
	PLATFORM_TYPE_TOP,
};

typedef struct {
	u8 type;
	Rect2D rect;
} Platform;

static struct {

	Platform platforms[MAX_STAGE_PLATFORMS];
	u8 nPlatforms;

} stages[NUM_STAGES] = {

	{
		{
			(Platform){PLATFORM_TYPE_TOP, {-2, 1, 4, 0.1}},
			(Platform){PLATFORM_TYPE_ONE_WAY, {-1.7, 2, 1.1, 0.1}},
			(Platform){PLATFORM_TYPE_ONE_WAY, {-0.5, 2.74, 1, 0.15}},
			(Platform){PLATFORM_TYPE_ONE_WAY, {0.65, 2, 1.1, 0.1}},
		}, 4
	},
};

void Stage_AddPlayer(Stage *stage, u8 index){

	Character_Create(&stage->players[stage->nPlayers], index);

	++stage->nPlayers;
}

void Stage_Init(Stage *stage, u8 stageIndex){

	stage->onStage = stageIndex;
}

void Stage_Close(Stage *stage){


	stage->nPlayers = 0;
}

void Stage_Update(Stage *stage){

	u8 k;
	for(k = 0; k < stage->nPlayers; k++)
		stage->players[k].Update(&stage->players[k]);
}