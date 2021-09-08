#include <malloc.h>
#include <assert.h>
#include "memory.h"

#define ALIGNMENT 8
#define ALIGN_UP(T, offset) (T)(((uintptr_t)offset + ALIGNMENT - 1) & ~(ALIGNMENT-1))
#define METADATA_SIZE ALIGNMENT
// #define METADATA_SIZE ALIGN_UP(u32, sizeof(u32))

static void *memory;
static void *stack_ends[2];
static void *stack_caps[2];

void memory_init(u32 size){

	size = ALIGN_UP(u32, size) - ALIGNMENT;
	
	memory = malloc(size + ALIGNMENT);

	stack_caps[STACK_BOTTOM] = ALIGN_UP(void *, memory);
	stack_caps[STACK_TOP] = ALIGN_UP(void *, memory + size);

	stack_ends[STACK_BOTTOM] = stack_caps[STACK_BOTTOM];
	stack_ends[STACK_TOP] = stack_caps[STACK_TOP];
}

void memory_close(void){

	free(memory);
}

void memory_stack_pop(u8 end, u16 num){

	u32 k;
	
	if(end){

		if(stack_ends[STACK_TOP] == stack_caps[STACK_TOP]) return;
	
		for(k = 0; k < num; k++)
			if(stack_ends[STACK_TOP] < stack_caps[STACK_TOP])
				stack_ends[STACK_TOP] += *((u32 *)stack_ends[STACK_TOP]);

	} else {

		if(stack_ends[STACK_BOTTOM] == stack_caps[STACK_BOTTOM]) return;

		for(k = 0; k < num; k++)
			if(stack_ends[STACK_BOTTOM] > stack_caps[STACK_BOTTOM])
				stack_ends[STACK_BOTTOM] -= *((u32 *)(stack_ends[STACK_BOTTOM] - METADATA_SIZE));
	}
}

void *memory_stack_alloc(u8 end, u32 size){

	size = ALIGN_UP(u32, size) + METADATA_SIZE;

#ifdef MEMORY_DEBUG
	assert((uintptr_t)stack_ends[STACK_BOTTOM] + size >= (uintptr_t)stack_ends[STACK_TOP]);
	assert((uintptr_t)stack_ends[STACK_TOP] - size <= (uintptr_t)stack_ends[STACK_BOTTOM]);
#endif

	void *mem = NULL;

	if(end){
		stack_ends[STACK_TOP] -= size;
		mem = stack_ends[STACK_TOP] + METADATA_SIZE;
		*((u32 *)stack_ends[STACK_TOP]) = size;

	} else {
		mem = stack_ends[STACK_BOTTOM];
		stack_ends[STACK_BOTTOM] += size - METADATA_SIZE;
		*((u32 *)stack_ends[STACK_BOTTOM]) = size;
		stack_ends[STACK_BOTTOM] += METADATA_SIZE;

	}

	return mem;
}

void *memory_stack_alloc_clear(u8 end, u32 size){

	u8 *mem = (u8 *)memory_stack_alloc(end, size);

	u32 k;
	for(k = 0; k < size; k++)
		mem[k] = 0;

	return mem;
}

void memory_stack_memcpy(void *restrict dest, const void *restrict from, u32 size){

	u8 *dest8 = (u8 *)dest;
	u8 *from8 = (u8 *)from;

	u32 k;
	for(k = 0; k < size; k++)
		dest8[k] = from8[k];
}