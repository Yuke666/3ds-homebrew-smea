#ifndef MEMORY_DEF
#define MEMORY_DEF

#include "server.h"

#define STACK_TOP 1
#define STACK_BOTTOM 0

#define MAIN_STACK STACK_BOTTOM
#define TEMP_STACK STACK_TOP

void memory_stack_pop(u8 end, u16 num);
void *memory_stack_alloc(u8 end, u32 size);
void *memory_stack_alloc_clear(u8 end, u32 size);
void memory_close(void);
void memory_init(u32 size);
void memory_stack_memcpy(void *restrict dest, const void *restrict from, u32 size);

#endif