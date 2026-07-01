#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
}

void writeChunk(Chunk* chunk, uint8_t byte) {
  if (chunk->count + 1 > chunk->capacity) {
    chunk->capacity = GROW_CAPACITY(chunk->capacity);
    uint8_t* new_code = reallocate(chunk->code, chunk->capacity);
    chunk->code = new_code;
  }
  chunk->code[chunk->count] = byte;
  chunk->count += 1;
}

void freeChunk(Chunk* chunk) {
  reallocate(chunk->code, chunk->capacity, 0);
  initChunk(chunk);
}
