#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
  Chunk* chunk;
  uint8_t* instruction_ptr;
  Value stack[STACK_MAX];
  Value* stack_top;
} VM;

typedef enum {
  INTERPRET_SUCCESS,
  INTERPRET_RUNTIME_ERROR,
  INTERPRET_COMPILE_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);
void push(Value v);
Value pop();

#endif
