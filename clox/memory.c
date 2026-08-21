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
    case OBJ_FUNCTION: {
      ObjFunction* f = (ObjFunction*)obj;
      freeChunk(&f->chunk);
      reallocate(f, sizeof(ObjFunction*), 0);
      break;
    }
    case OBJ_NATIVE: {
      ObjNative* n = (ObjNative*)obj;
      reallocate(n, sizeof(ObjNative*), 0);
      break;
    }
  }
}

void freeObjects() {
  Obj* object = vm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }
}
