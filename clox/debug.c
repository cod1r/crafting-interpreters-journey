#include <stdio.h>
#include "debug.h"
#include "chunk.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);
  for (int i = 0; i < chunk->count;) {
    i = disassembleInstruction(chunk, i);
  }
}

static int simpleInstruction(const char* opcode_name, int offset) {
  printf("%s\n", opcode_name);
  return offset + 1;
}

static int constantInstruction(
  const char* opcode_name, Chunk* chunk, int offset) {
  int constant_idx = chunk->code[offset + 1];
  printf("%-16s %4d '", opcode_name, constant_idx);
  printValue(chunk->constants.values[constant_idx]);
  printf("'\n");
  return offset + 2;
}

int disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);
  switch (chunk->code[offset]) {
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    default:
      printf("UNKNOWN INSTRUCTION %d\n", chunk->code[offset]);
      return offset + 1;
  }
}
