#include <stdlib.h>
#include "memory.h"
#include "object.h"
#include "vm.h"

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

static void freeObject(Obj* obj) {
  switch (obj->type) {
    case OBJ_STRING: {
      ObjString* s = (ObjString*)obj;
      reallocate(s->chars, sizeof(char) * s->length, 0);
      reallocate(s, sizeof(ObjString), 0);
      break;
    }
  }
}

void freeObjects() {
  Obj* next = vm.objects;
  while (next != NULL) {
    next = vm.objects->next;
    freeObject(vm.objects);
  }
}
