#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

static Obj* allocateObj(size_t size, ObjType type) {
  Obj* obj = reallocate(NULL, 0, size);
  obj->type = type;
  obj->next = vm.objects;
  vm.objects = obj;
  return obj;
}

ObjString* allocateString(char* chars, int length) {
  ObjString* obj = (ObjString*)allocateObj(sizeof(ObjString), OBJ_STRING);
  obj->chars = chars;
  obj->length = length;
  return obj;
}

ObjString* copyString(const char* chars, int length) {
  char* heapString = reallocate(NULL, 0, sizeof(char) * length);
  memcpy(heapString, chars, length);
  heapString[length] = '\0';
  return allocateString(heapString, length);
}

void printObject(Value v) {
  switch (v.as.obj->type) {
    case OBJ_STRING: {
      printf("\"%s\"", ((ObjString*)v.as.obj)->chars);
      break;
    }
  }
}
