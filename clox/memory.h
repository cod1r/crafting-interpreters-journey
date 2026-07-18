#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

#define GROW_CAPACITY(cap) (cap < 8 ? 8 : cap * 2)

void* reallocate(void* ptr, size_t oldSize, size_t newSize);

void freeObjects();

#endif
