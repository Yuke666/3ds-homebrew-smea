#include "sound.h"
#include "deflate.h"
#include <stdio.h>

#define SAMPLERATE 11025

int Sound_Load(Sound *sound, const char *path){

	FILE *fp = fopen(path, "rb");

	fread(&sound->size, 1, sizeof(int), fp);

	sound->samples = (u16 *)Memory_StackAlloc(MAIN_STACK,sound->size);

	fread(sound->samples, sound->size, 1, fp);

	// Deflate_Read(fp, sound->samples, sound->size);

	fclose(fp);

	return 0;
}

void Sound_Close(Sound *sound){

	Memory_StackPop(MAIN_STACK, 1);
}

void Sound_Play(u32 channel, Sound *sound){
	
 	GSPGPU_FlushDataCache(sound->samples, sound->size);
	
	csndPlaySound(channel, SOUND_ONE_SHOT | SOUND_FORMAT_16BIT, SAMPLERATE, 1.0, 0.0, 
		(u32*)sound->samples, NULL, sound->size);

}