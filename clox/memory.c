#include <stdlib.h>
#include "memory.h"

void* reallocate(void* ptr, size_t oldSize, size_t newSize) {
  void* newPtr = realloc(ptr, newSize);
  if (newSize == 0) {
    return NULL;
  }
  if (newPtr == NULL) {
    exit(1);
  }
  return newPtr;
}
