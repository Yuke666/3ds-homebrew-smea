#include <3ds.h>
#include "memory.h"

#define ALIGNMENT 16
#define ALIGN_UP(T, offset) (T)(((uintptr_t)offset + ALIGNMENT - 1) & ~(ALIGNMENT-1))

static void *memory;
static void *memoryBase;
static void *memoryCap;
static void *memoryUpper;
static void *memoryLower;

void Memory_Init(int size){

	size = ALIGN_UP(int, size) - ALIGNMENT;
	
	memory = linearAlloc(size + ALIGNMENT);

	memoryBase = ALIGN_UP(void *, memory);
	memoryCap = ALIGN_UP(void *, memory + size);

	memoryLower = memoryBase;
	memoryUpper = memoryCap;
}

void Memory_Close(void){

	linearFree(memory);
}

void Memory_StackPop(int end, int num){

	int k;
	
	if(end == STACK_TOP){

		if(memoryUpper == memoryCap) return;
	
		for(k = 0; k < num; k++)
			if(memoryUpper < memoryCap)
				memoryUpper += *((u32 *)memoryUpper);

	} else {

		if(memoryLower == memoryBase) return;

		for(k = 0; k < num; k++)
			if(memoryLower > memoryBase)
				memoryLower -= *((u32 *)(memoryLower - ALIGN_UP(int, sizeof(u32))));
	}
}

void *Memory_StackAlloc(int end, int size){

	size = ALIGN_UP(int, size) + ALIGN_UP(int, sizeof(u32));

	// assert((uintptr_t)memoryLower + size >= (uintptr_t)memoryUpper || (uintptr_t)memoryUpper - size <= (uintptr_t)memoryLower);

	void *mem = NULL;

	if(end == STACK_TOP){
		memoryUpper -= size;
		mem = memoryUpper + ALIGN_UP(int, sizeof(u32));
		*((u32 *)memoryUpper) = size;

	} else {
		mem = memoryLower;
		memoryLower += size - ALIGN_UP(int, sizeof(u32));
		*((u32 *)memoryLower) = size;
		memoryLower += ALIGN_UP(int, sizeof(u32));

	}

	return mem;
}

void *Memory_GetStack(int end){

	if(end == STACK_TOP)
		return memoryUpper;
	else
		return memoryLower;
}

void Memory_SetStack(int end, void *pos){

	if(end == STACK_TOP)
		memoryUpper = pos;
	else
		memoryLower = pos;
}