#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"
#include "object.h"
#include "table.h"

#define UINT8_COUNT (UINT8_MAX + 1)

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  ObjClosure* closure;
  uint8_t* instruction_ptr;
  Value* slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  Value stack[STACK_MAX];
  Value* stack_top;
  Table strings;
  Table globals;
  Obj* objects;
  ObjUpvalue* openUpvalues;
} VM;

extern VM vm;

typedef enum {
  INTERPRET_SUCCESS,
  INTERPRET_RUNTIME_ERROR,
  INTERPRET_COMPILE_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value v);
Value pop();

#endif
