#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "compiler.h"
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "memory.h"
#include "object.h"

VM vm;

static Value clockNative(int argCount, Value* args) {
  return number_value((double)clock() / CLOCKS_PER_SEC);
}

static void resetStack() {
  vm.stack_top = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
}

static void runtimeError(const char* fmt, ...) {
  va_list(args);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fputs("\n", stderr);
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    size_t instruction_idx =
      frame->instruction_ptr - frame->closure->function->chunk.code - 1;
    int line = frame->closure->function->chunk.lines[instruction_idx];
    fprintf(stderr, "[line %d] in \n", line);
    if (frame->closure->function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", frame->closure->function->name->chars);
    }
  }
  resetStack();
}

static void defineNative(const char* name, NativeFn fn) {
  push(object_value((Obj*)copyString(name, (int)strlen(name))));
  push(object_value((Obj*)newNative(fn)));
  tableSet(&vm.globals, (ObjString*)((vm.stack[0]).as.obj), vm.stack[1]);
  pop();
  pop();
}

void initVM() {
  initTable(&vm.strings);
  initTable(&vm.globals);
  resetStack();
  defineNative("clock", clockNative);
}

void freeVM() {
  freeTable(&vm.strings);
  freeTable(&vm.globals);
  freeObjects();
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
          case VALUE_OBJECT:
            switch (a.as.obj->type) {
              case OBJ_STRING: {
                ObjString* aString = (ObjString*)a.as.obj;
                ObjString* bString = (ObjString*)b.as.obj;
                push(bool_value(aString == bString));
                break;
              }
              case OBJ_FUNCTION: {
                ObjFunction* aF = (ObjFunction*)a.as.obj;
                ObjFunction* bF = (ObjFunction*)b.as.obj;
                push(bool_value(aF == bF));
                break;
              }
              case OBJ_NATIVE: {
                ObjNative* nt = (ObjNative*)a.as.obj;
                ObjNative* nt2 = (ObjNative*)b.as.obj;
                push(bool_value(nt == nt2));
                break;
              }
              case OBJ_CLOSURE: {
                ObjClosure* cl = (ObjClosure*)a.as.obj;
                ObjClosure* cl2 = (ObjClosure*)b.as.obj;
                push(bool_value(cl == cl2));
                break;
              }
              case OBJ_UPVALUE: break;
            }
        }
      }
      break;
    }
    case OP_GREATER:
      push(bool_value(a.as.number > b.as.number)); break;
    case OP_LESS:
      push(bool_value(a.as.number < b.as.number)); break;
    default: printf("UNKNOWN BINARY OP %d\n", op); exit(1);
  }
}

static void concatenate() {
  ObjString* b = (ObjString*)pop().as.obj;
  ObjString* a = (ObjString*)pop().as.obj;
  int newLength = a->length + b->length;
  char* newCString = reallocate(NULL, 0, sizeof(char) * newLength + 1);
  memcpy(newCString, a->chars, a->length);
  memcpy(newCString + a->length, b->chars, b->length);
  newCString[newLength] = '\0';

  uint32_t hash = hashString(newCString, newLength);
  ObjString* interned = tableFindString(&vm.strings, newCString, newLength,
                                        hash);
  if (interned != NULL) { push(object_value((Obj*)interned)); return; }

  push(object_value((Obj*)allocateString(newCString, newLength, hash)));
}

static bool isFalsey(Value v) {
  return v.type == VALUE_NIL ||
                (v.type == VALUE_BOOL && !v.as.boolean);
}

uint16_t read_short() {
  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  frame->instruction_ptr += 2;
  return (uint16_t)(frame->instruction_ptr[-2] << 8
                    | frame->instruction_ptr[-1]);
}

static bool call(ObjClosure* closure, uint8_t argCount) {
  if (closure->function->arity != argCount) {
    runtimeError("Expected %d arguments but got %d.",
      closure->function->arity, argCount);
    return false;
  }
  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }
  CallFrame* frame = &vm.frames[vm.frameCount++];
  frame->closure = closure;
  frame->instruction_ptr = closure->function->chunk.code;
  frame->slots = vm.stack_top - argCount - 1;
  return true;
}

static bool callValue(Value callee, uint8_t argCount) {
  if (callee.type == VALUE_OBJECT) {
    switch (callee.as.obj->type) {
      case OBJ_CLOSURE:
        return call((ObjClosure*)callee.as.obj, argCount);
        break;
      case OBJ_NATIVE: {
        NativeFn fn = ((ObjNative*)callee.as.obj)->function;
        Value result = fn(argCount, vm.stack_top - argCount);
        vm.stack_top -= argCount + 1;
        push(result);
        return true;
        break;
      }
      default: break;
    }
  }
  runtimeError("Can only call functions and classes.");
  return false;
}

static void closeUpvalues(Value* last) {
  while (vm.openUpvalues != NULL &&
        vm.openUpvalues->location >= last) {
    ObjUpvalue* upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = (ObjUpvalue*)upvalue->next;
  }
}

static ObjUpvalue* captureUpvalue(Value* value) {
  ObjUpvalue* prevUpvalue = NULL;
  ObjUpvalue* curr = vm.openUpvalues;
  while (curr != NULL && curr->location > value) {
    prevUpvalue = curr;
    curr = (ObjUpvalue*)curr->next;
  }
  if (curr != NULL && curr->location == value) {
    return curr;
  }
  ObjUpvalue* upvalue = newUpvalue(value);
  upvalue->next = (struct ObjUpvalue*)curr;
  if (prevUpvalue == NULL) vm.openUpvalues = upvalue;
  else prevUpvalue->next = (struct ObjUpvalue*)upvalue;
  return upvalue;
}

InterpretResult run() {
  while (true) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
#ifdef DEBUG_TRACE_EXECUTION
  printf("DEBUG STACK INFO:\n");
  if (vm.stack_top - vm.stack > 256) {
    printf("STACK OVERFLOW!!");
    return INTERPRET_RUNTIME_ERROR;
  }
  for (Value* v = vm.stack; v < vm.stack_top; ++v) {
    printf("[ ");
    printValue(*v);
    printf(" ]");
  }
  printf("\n");
  disassembleInstruction(&frame->closure->function->chunk,
    (int)(frame->instruction_ptr - frame->closure->function->chunk.code));
#endif
    uint8_t instruction;
    switch (instruction = *(frame->instruction_ptr++)) {
      case OP_CLOSE_UPVALUE: {
        closeUpvalues(vm.stack_top - 1);
        pop();
        break;
      }
      case OP_GET_UPVALUE: {
        uint8_t slot = *(frame->instruction_ptr++);
        push(*frame->closure->upvalues[slot]->location);
        break;
      }
      case OP_SET_UPVALUE: {
        uint8_t slot = *(frame->instruction_ptr++);
        *frame->closure->upvalues[slot]->location = peek(0);
        break;
      }
      case OP_CLOSURE: {
        ObjFunction* function =
          (ObjFunction*)frame->closure->function->chunk.constants.values[
            *(frame->instruction_ptr++)].as.obj;
        ObjClosure* closure = newClosure(function);
        push(object_value((Obj*)closure));
        for (int i = 0; i < closure->upvalueCount; ++i) {
          uint8_t isLocal = *(frame->instruction_ptr++);
          uint8_t idx = *(frame->instruction_ptr++);
          if (isLocal) {
            closure->upvalues[i] = captureUpvalue(&frame->slots[idx]);
          } else {
            closure->upvalues[i] = frame->closure->upvalues[idx];
          }
        }
        break;
      }
      case OP_CALL: {
        uint8_t argCount = *(frame->instruction_ptr++);
        if (!callValue(peek(argCount), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_LOOP: {
        uint16_t offset = read_short();
        frame->instruction_ptr -= offset;
        break;
      }
      case OP_JUMP: {
        uint16_t offset = read_short();
        frame->instruction_ptr += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = read_short();
        if (isFalsey(peek(0))) frame->instruction_ptr += offset;
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = *(frame->instruction_ptr++);
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name =
          (ObjString*)frame->closure->function->chunk.constants.values[*(frame->instruction_ptr++)].as.obj;
        if (tableSet(&vm.globals, name, peek(0))) {
          tableDelete(&vm.globals, name);
          runtimeError("Undefined global var '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_GET_LOCAL: {
        uint8_t slot = *(frame->instruction_ptr++);
        push(frame->slots[slot]);
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name =
          (ObjString*)frame->closure->function->chunk.constants.values[*(frame->instruction_ptr++)].as.obj;
        Value value;
        if (!tableGet(&vm.globals, name, &value)) {
          runtimeError("Undefined global var '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name =
          (ObjString*)frame->closure->function->chunk.constants.values[*(frame->instruction_ptr++)].as.obj;
        tableSet(&vm.globals, name,
                // peek not pop because tableSet can resize
                // vm might need to find value still so it needs to live
                // until after tableSet is done
                peek(0));
        pop();
        break;
      }
      case OP_POP:
        pop();
        break;
      case OP_PRINT:
        printValue(pop());
        printf("\n");
        break;
      case OP_RETURN: {
        Value result = pop();
        push(result);
        vm.frameCount--;
        if (vm.frameCount == 0) { pop(); return INTERPRET_SUCCESS; }
        vm.stack_top = frame->slots;
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_CONSTANT: {
        Value constant =
          frame->closure->function->chunk.constants.values[*(frame->instruction_ptr++)];
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
        push(bool_value(isFalsey(popped)));
        break;
      }
      case OP_ADD:
        if (peek(0).type == VALUE_OBJECT &&
            peek(1).type == VALUE_OBJECT &&
            isObjType(peek(0), OBJ_STRING) &&
            isObjType(peek(1), OBJ_STRING)) {
          concatenate();
          break;
        } else if (
                  (peek(0).type != peek(1).type) ||
                  (peek(0).type == VALUE_OBJECT &&
                   peek(1).type == VALUE_OBJECT &&
                   peek(0).as.obj->type != peek(1).as.obj->type)) {
          runtimeError("Operands must both be strings or both be numbers.");
          return INTERPRET_RUNTIME_ERROR;
        }
      case OP_EQUAL:
        handle_binary_op(instruction);
        break;
      case OP_LESS:
      case OP_GREATER:
      case OP_SUBTRACT:
      case OP_MULTIPLY:
      case OP_DIVIDE:
        if (peek(0).type != VALUE_NUMBER || peek(1).type != VALUE_NUMBER) {
          runtimeError("Operands must both be strings or both be numbers.");
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
  ObjFunction* function = compile(source);
  if (function == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }
  push(object_value((Obj*)function));
  ObjClosure* closure = newClosure(function);
  call(closure, 0);

  InterpretResult result = run();
  return result;
}
