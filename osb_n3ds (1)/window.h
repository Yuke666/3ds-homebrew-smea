#ifndef WINDOW_DEF
#define WINDOW_DEF

#include <3ds.h>
#include <citro3d.h>
#include "math.h"

#define DEBUG 						1
#define TOP_WIDTH 					400
#define TOP_HEIGHT 					240
#define BOTTOM_WIDTH 				320
#define BOTTOM_HEIGHT 				240

typedef struct Game Game;

struct Game {
	void 	*data;
	void 	*initialStackPos;
	void 	(*Close)(Game *game);
	void 	(*RenderTop)(Game *game);
	void 	(*RenderBottom)(Game *game);
	void 	(*Update)(Game *game);
};

typedef struct {
	shaderProgram_s 	program;
	DVLB_s 				*dvlb;
	C3D_AttrInfo 		attrInfo;
	int 				projViewLoc;
	int 				modelLoc;
	int 				bonesLoc;
} Shader;


extern Shader skinnedShader;
extern Shader standard2DShader;
extern Shader standard3DShader;
extern Shader textureless3DShader;

float GetDeltaTime(void);
u64 GetCurrTime(void);
void BindShader(Shader *shader);
void DrawRect(Rect2D screenRect, Rect2D imgRect, C3D_Tex *tex);
void GetSocketErrorMessage(char *into);
int GetServerSocket(void);

#endif