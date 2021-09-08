#include "utils.h"
#include "deflate.h"

void Utils_SetUniformMatrix(C3D_FVec *rows, float *matrix){

	int k;
	for(k = 0; k < 4; k++){
		rows[k].x = matrix[(k*4)];
		rows[k].y = matrix[(k*4)+1];
		rows[k].z = matrix[(k*4)+2];
		rows[k].w = matrix[(k*4)+3];
	}
}

void Utils_LoadImage(C3D_Tex *tex, const char *path, int filter, int channels){

	FILE *fp = fopen(path, "rb");

	int w, h;
	fread(&w, 1, sizeof(int), fp);
	fread(&h, 1, sizeof(int), fp);

	C3D_TexInit(tex, w, h, channels == 4 ? GPU_RGBA8 : GPU_RGB8);

	Deflate_Read(fp, tex->data, w * h * channels);

	C3D_TexSetFilter(tex, filter, GPU_NEAREST);
	C3D_TexSetWrap(tex, GPU_REPEAT, GPU_REPEAT);

	fclose(fp);
}

void Utils_Vertex22Rect(Vertex22 *out, Rect2D screenRect, Rect2D imgRect){
    
    out[0].pos = (Vec2){ screenRect.x, screenRect.y};
    out[0].coord = (Vec2){ imgRect.x, imgRect.y+imgRect.h};
    out[1].pos = (Vec2){ screenRect.x, screenRect.y+screenRect.h};
    out[1].coord = (Vec2){ imgRect.x, imgRect.y};
    out[2].pos = (Vec2){ screenRect.x+screenRect.w, screenRect.y+screenRect.h};
    out[2].coord = (Vec2){ imgRect.x+imgRect.w, imgRect.y};
    out[3].pos = (Vec2){ screenRect.x+screenRect.w, screenRect.y+screenRect.h};
    out[3].coord = (Vec2){ imgRect.x+imgRect.w, imgRect.y};
    out[4].pos = (Vec2){ screenRect.x+screenRect.w, screenRect.y};
    out[4].coord = (Vec2){ imgRect.x+imgRect.w, imgRect.y+imgRect.h};
    out[5].pos = (Vec2){ screenRect.x, screenRect.y};
    out[5].coord = (Vec2){ imgRect.x, imgRect.y+imgRect.h};
}