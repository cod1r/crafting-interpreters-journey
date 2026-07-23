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
  obj->next = vm.objects;
  vm.objects = obj;
  return obj;
}

ObjString* allocateString(char* chars, int length, uint32_t hash) {
  ObjString* obj = (ObjString*)allocateObj(sizeof(ObjString), OBJ_STRING);
  obj->chars = chars;
  obj->length = length;
  obj->hash = hash;
  tableSet(&vm.strings, obj, nil_value());
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

void printObject(Value v) {
  switch (v.as.obj->type) {
    case OBJ_STRING: {
      printf("\"%s\"", ((ObjString*)v.as.obj)->chars);
      break;
    }
  }
}
