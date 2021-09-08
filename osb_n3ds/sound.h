#ifndef SOUND_DEF
#define SOUND_DEF

#include <3ds.h>
#include "memory.h"

typedef struct {
	u16 *samples; 
	int size;
	int startTime;
} Sound;

int Sound_Load(Sound *sound, const char *path);
void Sound_Close(Sound *sound);
void Sound_Play(u32 channel, Sound *sound);

#endif