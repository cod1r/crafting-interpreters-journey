#ifndef clox_value_h
#define clox_value_h
#include "common.h"

typedef enum {
  VALUE_BOOL,
  VALUE_NIL,
  VALUE_NUMBER,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as;
} Value;

Value bool_value(bool value);

Value nil_value();

Value number_value(double value);

typedef struct {
  int count;
  int capacity;
  Value* values;
} ValueArray;

void initValueArr(ValueArray* arr);
void writeValueArr(ValueArray* arr, Value val);
void freeValueArr(ValueArray* arr);
void printValue(Value v);

#endif
