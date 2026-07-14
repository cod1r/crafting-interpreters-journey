#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "compiler.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void resetStack() {
  vm.stack_top = vm.stack;
}

static void runtimeError(const char* fmt, ...) {
  va_list(args);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fputs("\n", stderr);
  size_t instruction_idx = vm.instruction_ptr - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction_idx];
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
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

static Value peek(int distance) {
  return *(vm.stack_top + (-1 - distance));
}

void handle_binary_op(uint8_t op) {
  Value b = pop();
  Value a = pop();
  switch (op) {
    case OP_ADD:
      push(number_value(a.as.number + b.as.number)); break;
    case OP_SUBTRACT:
      push(number_value(a.as.number - b.as.number)); break;
    case OP_MULTIPLY:
      push(number_value(a.as.number * b.as.number)); break;
    case OP_DIVIDE:
      push(number_value(a.as.number / b.as.number)); break;
    case OP_EQUAL: {
      if (a.type != b.type) {
        push(bool_value(false));
      } else {
        switch (a.type) {
          case VALUE_NIL: push(bool_value(true)); break;
          case VALUE_NUMBER:
            push(bool_value(a.as.number == b.as.number));
            break;
          case VALUE_BOOL:
            push(bool_value(a.as.boolean == b.as.boolean));
            break;
        }
      }
      break;
    }
    case OP_GREATER: push(bool_value(a.as.number > b.as.number)); break;
    case OP_LESS: push(bool_value(a.as.number < b.as.number)); break;
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
      case OP_NIL: push(nil_value()); break;
      case OP_TRUE: push(bool_value(true)); break;
      case OP_FALSE: push(bool_value(false)); break;
      case OP_NEGATE:
        if (peek(0).type != VALUE_NUMBER) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(number_value(-pop().as.number));
        break;
      case OP_NOT: {
        Value popped = pop();
        push(bool_value(popped.type == VALUE_NIL ||
              (popped.type == VALUE_BOOL && !popped.as.boolean)));
        break;
      }
      case OP_EQUAL:
        handle_binary_op(instruction);
        break;
      case OP_LESS:
      case OP_GREATER:
      case OP_ADD:
      case OP_SUBTRACT:
      case OP_MULTIPLY:
      case OP_DIVIDE:
        if (peek(0).type != VALUE_NUMBER || peek(1).type != VALUE_NUMBER) {
          runtimeError("Operands must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        handle_binary_op(instruction);
        break;
      default:
        printf("UNKNOWN OPCODE %4d\n", instruction);
        exit(1);
    }
  }
}

InterpretResult interpret(const char* source) {
  Chunk chunk;
  initChunk(&chunk);

  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }
  vm.chunk = &chunk;
  vm.instruction_ptr = chunk.code;

  InterpretResult result = run();
  freeChunk(&chunk);
  return result;
}
