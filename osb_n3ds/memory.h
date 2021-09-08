#ifndef MEMORY_DEF
#define MEMORY_DEF

#define STACK_TOP 1
#define STACK_BOTTOM 0

#define MAIN_STACK STACK_BOTTOM
#define TEMP_STACK STACK_TOP

#define MEMORY_DEBUG 1

void Memory_StackPop(int end, int num);
void *Memory_StackAlloc(int end, int size);
void *Memory_GetStack(int end);
void Memory_SetStack(int end, void *pos);
void Memory_Close(void);
void Memory_Init(int size);

#endif