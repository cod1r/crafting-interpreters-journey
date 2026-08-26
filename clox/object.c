#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

static Obj* allocateObj(size_t size, ObjType type) {
  Obj* obj = reallocate(NULL, 0, size);
  obj->type = type;
  obj->isMarked = false;
  obj->next = vm.objects;
  vm.objects = obj;
#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*)obj, size, type);
#endif
  return obj;
}

ObjString* allocateString(char* chars, int length, uint32_t hash) {
  ObjString* obj = (ObjString*)allocateObj(sizeof(ObjString), OBJ_STRING);
  obj->chars = chars;
  obj->length = length;
  obj->hash = hash;
  push(object_value((Obj*)obj));
  tableSet(&vm.strings, obj, nil_value());
  pop();
  return obj;
}

uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString* copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&vm.strings, chars, length,
                                        hash);
  if (interned != NULL) return interned;
  char* heapString = reallocate(NULL, 0, sizeof(char) * length + 1);
  memcpy(heapString, chars, length);
  heapString[length] = '\0';
  return allocateString(heapString, length, hash);
}

static void printFunction(ObjFunction* function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

void printObject(Value v) {
  switch (v.as.obj->type) {
    case OBJ_STRING: {
      printf("\"%s\"", ((ObjString*)v.as.obj)->chars);
      break;
    }
    case OBJ_FUNCTION: {
      printFunction((ObjFunction*)v.as.obj);
      break;
    }
    case OBJ_NATIVE: {
      printf("<native function>");
      break;
    }
    case OBJ_CLOSURE: {
      printFunction(((ObjClosure*)v.as.obj)->function);
      break;
    }
    case OBJ_UPVALUE: {
      printf("upvalue");
      break;
    }
    case OBJ_CLASS: {
      printf("%s", ((ObjClass*)v.as.obj)->name->chars);
      break;
    }
    case OBJ_INSTANCE: {
      printf("%s instance", ((ObjInstance*)v.as.obj)->class_->name->chars);
      break;
    }
    case OBJ_BOUND_METHOD: {
      printFunction(((ObjBoundMethod*)v.as.obj)->method->function);
      break;
    }
  }
}

ObjFunction* newFunction() {
  ObjFunction* function = (ObjFunction*)allocateObj(sizeof(ObjFunction), OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  function->upvalueCount = 0;
  initChunk(&function->chunk);
  return function;
}

ObjNative* newNative(NativeFn function) {
  ObjNative* nativeObj = (ObjNative*)allocateObj(sizeof(ObjNative), OBJ_NATIVE);
  nativeObj->function = function;
  return nativeObj;
}

ObjClosure* newClosure(ObjFunction* function) {
  ObjUpvalue** upvalues = reallocate(NULL, 0, sizeof(ObjUpvalue*) *
    function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }
  ObjClosure* closure = (ObjClosure*)allocateObj(sizeof(ObjClosure), OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjUpvalue* newUpvalue(Value* location) {
  ObjUpvalue* upvalue = (ObjUpvalue*)allocateObj(sizeof(ObjUpvalue), OBJ_UPVALUE);
  upvalue->location = location;
  upvalue->next = NULL;
  upvalue->closed = nil_value();
  return upvalue;
}

ObjClass* newClass(ObjString* name) {
  ObjClass* class_ = (ObjClass*)allocateObj(sizeof(ObjClass), OBJ_CLASS);
  class_->name = name;
  initTable(&class_->methods);
  return class_;
}

ObjInstance* newInstance(ObjClass* class_) {
  ObjInstance* instance = (ObjInstance*)allocateObj(sizeof(ObjInstance), OBJ_INSTANCE);
  instance->class_ = class_;
  initTable(&instance->fields);
  return instance;
}

ObjBoundMethod* newBoundMethod(Value receiver,
                                ObjClosure* method) {
  ObjBoundMethod* bound = (ObjBoundMethod*)allocateObj(
    sizeof(ObjBoundMethod),
    OBJ_BOUND_METHOD
  );
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}
