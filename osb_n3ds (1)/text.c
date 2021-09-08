#include "text.h"
#include "images.h"
#include "window.h"
#include "math.h"
#include "utils.h"

#define TAB_SPACING 4
#define MAX_DRAW_CHARACTERS 512
#define SS_CHAR_SIZE 0.0625f

static char *fontPaths[] = {
    IMAGE_FONT,
};

static const Vertex22 squareData[6] = {
    {{-0.5f, -0.5f }, { 0, SS_CHAR_SIZE }},
    {{-0.5f, 0.5f }, { 0, 0 }},
    {{0.5f, 0.5f}, { SS_CHAR_SIZE, 0}},
    {{0.5f, 0.5f}, { SS_CHAR_SIZE, 0}},
    {{0.5f, -0.5f}, { SS_CHAR_SIZE, SS_CHAR_SIZE }},
    {{-0.5f, -0.5f}, { 0, SS_CHAR_SIZE }},
};

static Vertex22 *vboData;
static C3D_BufInfo bufInfo;
static C3D_Tex textures[NUM_FONTS];
static int onCharacter;

void Text_Close(void){

    int k;
    for(k = 0; k < NUM_FONTS; k++)
        C3D_TexDelete(&textures[k]);

    linearFree(vboData);
}

void Text_Init(void){

    int k;
    for(k = 0; k < NUM_FONTS; k++)
        Utils_LoadImage(&textures[k], fontPaths[k], GPU_NEAREST, 4);

    vboData = (Vertex22 *)linearAlloc(sizeof(squareData) * MAX_DRAW_CHARACTERS);

    BufInfo_Init(&bufInfo);
    BufInfo_Add(&bufInfo, (void *)vboData, sizeof(Vertex22), 2, 0x10);

    onCharacter = 0;
}

static int ValidateXY(int *x, int *y, int fontSize, int startX, int vSpacing, int maxWidth){
    
    // bottom width is the smaller one.
    if(*x+fontSize >= maxWidth - fontSize){

        *y += (fontSize + vSpacing);
        *x = startX;

        // top and bottom height are the same.
        if(*y+fontSize >= TOP_HEIGHT)
            return -1;
    }

    return 1;
}

void Text_Draw(int font, int x, int y, int hSpacing, int vSpacing, int maxWidth, const char *text){


    if(onCharacter >= MAX_DRAW_CHARACTERS)
        return;

    float fontSize = textures[font].width / 16;

    float startX = x;

    int num = 0;

    int index = 0;
    
    while(1){

        int p = text[index++];

        if(!p) break;

        if(p == '\n'){

            y += (fontSize + vSpacing);
            
            x = startX;

            // top and bottom height are the same.
            if(y+fontSize >= TOP_HEIGHT)
                break;

            continue;
        
        } else if(p == '\t' ){
            
            x += (fontSize + hSpacing) * TAB_SPACING;
            
            if(ValidateXY(&x, &y, fontSize, startX, vSpacing, maxWidth) < 0) break;

            continue;

        } else if(p < 32){

            break;
        
        } else if(p == 32){

            x += (fontSize + hSpacing);

            if(ValidateXY(&x, &y, fontSize, startX, vSpacing, maxWidth) < 0) break;

            continue;
        }

        p -= 32;

        float tX = (p % 16) / 16.0f;
        float tY = (1 - SS_CHAR_SIZE) - (floorf(p / 16.0f) / 16.0f);

        Vertex22 *data = &vboData[(num+onCharacter) * 6];

        int k;
        for(k = 0; k < 6; k++){

            data[k].coord.x = (squareData[k].coord.x) + tX;
            data[k].coord.y = (squareData[k].coord.y) + tY;
            data[k].pos.x = (squareData[k].pos.x * fontSize) + x;
            data[k].pos.y = (squareData[k].pos.y * fontSize) + y;
        }

        x += (fontSize + hSpacing);

        ValidateXY(&x, &y, fontSize, startX, vSpacing, maxWidth);

        ++num;

        if(num+onCharacter >= MAX_DRAW_CHARACTERS)
            break;

    }

    C3D_SetBufInfo(&bufInfo);
    C3D_TexBind(0, &textures[font]);

    C3D_DrawArrays(GPU_TRIANGLES, 6 * onCharacter, 6 * num);

    onCharacter += num;
}

void Text_Clear(void){

    onCharacter = 0;
}