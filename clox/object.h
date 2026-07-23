#ifndef clox_object_h
#define clox_object_h
#include "common.h"
#include "value.h"

typedef enum {
  OBJ_STRING
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

static inline bool isObjType(Value val, ObjType type) {
  return val.type == VALUE_OBJECT && val.as.obj->type == OBJ_STRING;
}

ObjString* copyString(const char* chars, int length);
ObjString* allocateString(char* chars, int length, uint32_t hash);
uint32_t hashString(const char* chars, int length);

void printObject(Value v);

#endif
