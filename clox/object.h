#ifndef clox_object_h
#define clox_object_h
#include "common.h"
#include "chunk.h"
#include "value.h"

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_NATIVE
} ObjType;

struct Obj {
  ObjType type;
  Obj* next;
};

struct ObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
};

typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

static inline bool isObjType(Value val, ObjType type) {
  return val.type == VALUE_OBJECT && val.as.obj->type == type;
}

ObjString* copyString(const char* chars, int length);
ObjString* allocateString(char* chars, int length, uint32_t hash);
uint32_t hashString(const char* chars, int length);
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);

void printObject(Value v);

#endif
