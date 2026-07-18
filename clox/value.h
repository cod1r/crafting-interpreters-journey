#ifndef clox_value_h
#define clox_value_h
#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
  VALUE_BOOL,
  VALUE_NIL,
  VALUE_NUMBER,
  VALUE_OBJECT
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj* obj;
  } as;
} Value;

Value bool_value(bool value);

Value nil_value();

Value number_value(double value);

Value object_value(Obj* obj);

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
