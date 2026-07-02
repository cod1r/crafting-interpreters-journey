#include <stdio.h>
#include "value.h"
#include "common.h"
#include "memory.h"

void initValueArr(ValueArray* arr) {
  arr->count = 0;
  arr->capacity = 0;
  arr->values = NULL;
}

void writeValueArr(ValueArray* arr, Value val) {
  if (arr->count + 1 > arr->capacity) {
    size_t oldCap = arr->capacity;
    arr->capacity = GROW_CAPACITY(arr->capacity);
    Value* new_values = reallocate(
      arr->values, sizeof(Value) * oldCap, sizeof(Value) * arr->capacity);
    arr->values = new_values;
  }
  arr->values[arr->count] = val;
  arr->count += 1;
}

void freeValueArr(ValueArray* arr) {
  reallocate(arr->values, sizeof(Value) * arr->capacity, 0);
  initValueArr(arr);
}

void printValue(Value v) { printf("%g", v); }
