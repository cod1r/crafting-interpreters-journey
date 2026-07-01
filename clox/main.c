#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"

int main(int argc, char **argv) {
  Chunk chunk;
  initChunk(&chunk);
  int constant_idx = addConstant(&chunk, 1.2);
  writeChunk(&chunk, OP_CONSTANT);
  writeChunk(&chunk, constant_idx);
  writeChunk(&chunk, OP_RETURN);
  disassembleChunk(&chunk, "example chunk");
  freeChunk(&chunk);
  return 0;
}
