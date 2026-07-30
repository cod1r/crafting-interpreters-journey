#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"
#include "object.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
  Chunk* chunk;
  uint8_t* instruction_ptr;
  Value stack[STACK_MAX];
  Value* stack_top;
  Table strings;
  Table globals;
  Obj* objects;
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
