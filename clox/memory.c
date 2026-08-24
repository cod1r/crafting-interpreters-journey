#include <stdlib.h>
#include "memory.h"
#include "object.h"
#include "vm.h"
#include "compiler.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* ptr, size_t oldSize, size_t newSize) {
  vm.bytesAllocated += newSize - oldSize;
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif
    if (vm.bytesAllocated > vm.nextGC) {
      collectGarbage();
    }
  }
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
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)obj, obj->type);
#endif
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
    case OBJ_CLOSURE: {
      ObjClosure* c = (ObjClosure*)obj;
      reallocate(c->upvalues, sizeof(ObjUpvalue*) * c->upvalueCount, 0);
      reallocate(c, sizeof(ObjClosure), 0);
      break;
    }
    case OBJ_UPVALUE: {
      ObjUpvalue* upv = (ObjUpvalue*)obj;
      reallocate(upv, sizeof(ObjUpvalue), 0);
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
  free(vm.grayStack);
}

void markObject(Obj* obj) {
  if (obj == NULL) return;
  if (obj->isMarked) return;
#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)obj);
  printValue(object_value(obj));
  printf("\n");
#endif
  obj->isMarked = true;

  if (vm.grayCapacity < vm.grayCount + 1) {
    vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
    vm.grayStack = (Obj**)realloc(vm.grayStack,
                    sizeof(Obj*) * vm.grayCapacity);
    if (vm.grayStack == NULL) exit(1);
  }
  vm.grayStack[vm.grayCount++] = obj;
}

void markValue(Value v) {
  if (v.type == VALUE_OBJECT) markObject(v.as.obj);
}

static void markRoots() {
  for (Value* v = vm.stack; v < vm.stack_top; v++) {
    markValue(*v);
  }
  for (int i = 0; i < vm.frameCount; ++i) {
    markObject((Obj*)vm.frames[i].closure);
  }
  for (ObjUpvalue* upv = vm.openUpvalues;
        upv != NULL; upv = (ObjUpvalue*)upv->next) {
    markObject((Obj*)upv);
  }
  markCompilerRoots();
  markTable(&vm.globals);
}

static void markArray(ValueArray* array) {
  for (int i = 0; i < array->count; ++i) {
    markValue(array->values[i]);
  }
}

static void blackenObject(Obj* obj) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)obj);
  printValue(object_value(obj));
  printf("\n");
#endif
  switch (obj->type) {
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)obj;
      markObject((Obj*)closure->function);
      for (int i = 0; i < closure->upvalueCount; ++i) {
        markObject((Obj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* fn = (ObjFunction*)obj;
      markObject((Obj*)fn->name);
      markArray(&fn->chunk.constants);
      break;
    }
    case OBJ_UPVALUE:
      markValue(((ObjUpvalue*)obj)->closed);
      break;
    case OBJ_STRING:
    case OBJ_NATIVE:
      break;
  }
}

static void traceReferences() {
  while (vm.grayCount > 0) {
    Obj* obj = vm.grayStack[--vm.grayCount];
    blackenObject(obj);
  }
}

static void sweep() {
  Obj* prev = NULL;
  Obj* curr = vm.objects;
  while (curr != NULL) {
    Obj* obj = curr;
    if (obj->isMarked) {
      obj->isMarked = false;
      prev = curr;
      curr = curr->next;
    } else {
      Obj* unreached = curr;
      curr = curr->next;
      if (prev == NULL) {
        vm.objects = curr;
      } else {
        prev->next = curr;
      }
      freeObject(unreached);
    }
  }
}

static void tableRemoveWhite(Table* table) {
  for (int i = 0; i < table->capacity; ++i) {
    Entry* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->obj.isMarked) {
      tableDelete(table, entry->key);
    }
  }
}

void collectGarbage() {
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = vm.bytesAllocated;
#endif
  markRoots();
  traceReferences();
  sweep();

  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
    before - vm.bytesAllocated, before, vm.bytesAllocated,
    vm.nextGC);
#endif
}
