#include <stdlib.h>
#include "chunk.h"
#include "memory.h"
#include "value.h"

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initValueArr(&chunk->constants);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  if (chunk->count + 1 > chunk->capacity) {
    size_t oldCap = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(chunk->capacity);
    uint8_t* new_code = reallocate(
      chunk->code, sizeof(uint8_t) * oldCap, chunk->capacity);
    int* new_lines = reallocate(
      chunk->lines, sizeof(int) * oldCap, chunk->capacity);
    chunk->code = new_code;
    chunk->lines = new_lines;
  }
  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count += 1;
}

void freeChunk(Chunk* chunk) {
  reallocate(chunk->code, sizeof(uint8_t) * chunk->capacity, 0);
  reallocate(chunk->lines, sizeof(int) * chunk->capacity, 0);
  freeValueArr(&chunk->constants);
  initChunk(chunk);
}

int addConstant(Chunk* chunk, Value val) {
  writeValueArr(&chunk->constants, val);
  return chunk->constants.count - 1;
}
