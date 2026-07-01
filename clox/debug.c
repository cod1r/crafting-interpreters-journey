#include <stdio.h>
#include "debug.h"
#include "chunk.h"

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

int disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);
  switch (chunk->code[offset]) {
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    default:
      printf("UNKNOWN INSTRUCTION %d\n", chunk->code[offset]);
      return offset + 1;
  }
}
