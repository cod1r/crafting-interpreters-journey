#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "vm.h"

int main(int argc, char **argv) {
  initVM();
  Chunk chunk;
  initChunk(&chunk);
  int constant_idx = addConstant(&chunk, 1.2);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant_idx, 123);
  writeChunk(&chunk, OP_RETURN, 123);
  disassembleChunk(&chunk, "example chunk");
  interpret(&chunk);
  freeChunk(&chunk);
  freeVM();
  return 0;
}
