#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "vm.h"

static void repl() {
  char line[1024];
  while (true) {
    printf("> ");

    if (fgets(line, sizeof(line), stdin) == NULL) {
      printf("\n");
      break;
    }

    interpret(line);
  }
}

static char* readFile(const char* filePath) {
  FILE* file = fopen(filePath, "r");
  if (file == NULL) {
    fprintf(stderr, "Cannot open file \"%s\"\n", filePath);
    exit(74);
  }

  fseek(file, 0, SEEK_END);
  size_t fileSize = ftell(file);

  rewind(file);

  char* source = malloc(fileSize + 1);
  if (source == NULL) {
    fprintf(stderr, "Not enough memory to read file \"%s\"\n", filePath);
    exit(74);
  }
  size_t bytes_read = fread(source, sizeof(char), fileSize, file);
  if (bytes_read < fileSize) {
    fprintf(stderr, "Could not read entire file \"%s\"\n", filePath);
    exit(74);
  }
  source[bytes_read] = '\0';
  fclose(file);
  return source;
}

static void runFile(const char* file) {
  char* source = readFile(file);
  InterpretResult result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, char **argv) {
  initVM();

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: clox [path]");
    exit(64);
  }

  freeVM();
  return 0;
}
