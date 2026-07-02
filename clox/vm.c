#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void resetStack() {
  vm.stack_top = vm.stack;
}

void initVM() {
  vm.chunk = NULL;
  vm.instruction_ptr = NULL;
  resetStack();
}

void freeVM() {
}

void push(Value v) {
  *vm.stack_top = v;
  vm.stack_top++;
}

Value pop() {
  return *(--vm.stack_top);
}

void handle_binary_op(uint8_t op) {
  Value b = pop();
  Value a = pop();
  switch (op) {
    case OP_ADD: push(a + b); break;
    case OP_SUBTRACT: push(a - b); break;
    case OP_MULTIPLY: push(a * b); break;
    case OP_DIVIDE: push(a / b); break;
    default: printf("UNKNOWN BINARY OP %d\n", op); exit(1);
  }
}

InterpretResult run() {
  while (true) {
#ifdef DEBUG_TRACE_EXECUTION
  printf("DEBUG STACK INFO:\n");
  for (Value* v = vm.stack; v < vm.stack_top; ++v) {
    printf("[ ");
    printValue(*v);
    printf(" ]");
  }
  printf("\n");
  disassembleInstruction(vm.chunk,
    (int)(vm.instruction_ptr - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = *(vm.instruction_ptr++)) {
      case OP_RETURN:
        printValue(pop());
        printf("\n");
        return INTERPRET_SUCCESS;
      case OP_CONSTANT: {
        Value constant =
          vm.chunk->constants.values[*(vm.instruction_ptr++)];
        push(constant);
        break;
      }
      case OP_NEGATE:
        push(-pop());
        break;
      case OP_ADD:
      case OP_SUBTRACT:
      case OP_MULTIPLY:
      case OP_DIVIDE:
        handle_binary_op(instruction);
        break;
      default:
        printf("UNKNOWN OPCODE %4d\n", instruction);
        exit(1);
    }
  }
}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.instruction_ptr = chunk->code;
  return run();
}
