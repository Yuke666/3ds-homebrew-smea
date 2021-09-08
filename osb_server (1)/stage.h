#ifndef STAGE_DEF
#define STAGE_DEF

#include "characters.h"
#include "server.h"

#define MAX_PLAYERS 4

typedef struct {
	Character players[MAX_PLAYERS];
	u8 nPlayers;
	u8 onStage;
} Stage;

enum {
	STAGE_SPACE = 0,
	NUM_STAGES,
};

void Stage_Init(Stage *stage, u8 stageIndex);
void Stage_Close(Stage *stage);
void Stage_Update(Stage *stage);
void Stage_AddPlayer(Stage *stage, u8 character);


#endif