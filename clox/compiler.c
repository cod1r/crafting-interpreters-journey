#include <stdio.h>
#include "compiler.h"
#include "common.h"
#include "scanner.h"
#include "chunk.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token prev;
  Token curr;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef struct {
  void (*prefix)();
  void (*infix)();
  Precedence prec;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk() {
  return compilingChunk;
}

static void errorAt(Token* t, const char* msg) {
  if (parser.panicMode) return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] error", t->line);

  if (t->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else {
    fprintf(stderr, " at '%.*s'", t->length, t->start);
  }
  fprintf(stderr, ": %s\n", msg);
  parser.hadError = true;
}

static void error(const char* msg) {
  errorAt(&parser.prev, msg);
}

static void errorAtCurrent(const char* msg) {
  errorAt(&parser.curr, msg);
}

static void advance() {
  parser.prev = parser.curr;
  while (true) {
    parser.curr = scanToken();
    if (parser.curr.type != TOKEN_ERROR) break;
    errorAtCurrent(parser.curr.start);
  }
}

static void consume(TokenType type, const char* msg) {
  if (parser.curr.type == type) {
    advance();
    return;
  }
  errorAtCurrent(msg);
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.curr.line);
}

static void emitOpCodeOperand(uint8_t opcode, uint8_t operand) {
  emitByte(opcode);
  emitByte(operand);
}

static void emitReturn() {
  emitByte(OP_RETURN);
}

static void endCompiler() {
  emitReturn();
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}

static uint8_t makeConstant(Value v) {
  int constant_idx = addConstant(currentChunk(), v);
  if (constant_idx > UINT8_MAX) {
    error("Too many constants in one chunk");
    return 0;
  }
  return constant_idx;
}

static void emitConstant(Value v) {
  emitOpCodeOperand(OP_CONSTANT, makeConstant(v));
}

static Value convert_to_value() {
  const char* str = parser.prev.start;
  double v = 0.0;
  double multiplier = 1;
  bool past_decimal_point = false;
  for (int i = 0; i < parser.prev.length; ++i) {
    if (str[i] == '.') {
      past_decimal_point = true;
      continue;
    }
    if (past_decimal_point) {
      multiplier = multiplier > 1 ? 0.1 : multiplier / 10;
      v += (double)(str[i] - '0') * multiplier;
    } else {
      v *= 10;
      v += (double)(str[i] - '0') * multiplier;
      multiplier *= 10;
    }
  }
  return number_value(v);
}

static ParseRule* getRule(TokenType token);

static void parsePrecedence(Precedence p) {
  advance();
  void (*prefixRule)() = getRule(parser.prev.type)->prefix;
  if (prefixRule == NULL) {
    error("Expected expression.");
    return;
  }
  prefixRule();
  while (p <= getRule(parser.curr.type)->prec) {
    advance();
    void (*infixRule)() = getRule(parser.prev.type)->infix;
    infixRule();
  }
}

static void binary() {
  TokenType binaryOp = parser.prev.type;
  ParseRule* rule = getRule(binaryOp);
  parsePrecedence((Precedence)rule->prec + 1);
  switch (binaryOp) {
    case TOKEN_PLUS: emitByte(OP_ADD); break;
    case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
    case TOKEN_BANG_EQUAL: emitOpCodeOperand(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
    case TOKEN_LESS: emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL: emitOpCodeOperand(OP_GREATER, OP_NOT); break;
    case TOKEN_GREATER: emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitOpCodeOperand(OP_LESS, OP_NOT); break;
    default: return;
  }
}

static void unary() {
  TokenType unaryOp = parser.prev.type;

  parsePrecedence(PREC_UNARY);

  switch (unaryOp) {
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    case TOKEN_BANG: emitByte(OP_NOT); break;
    default: return;
  }
}

static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
  Value v = convert_to_value();
  emitConstant(v);
}

static void literal() {
  switch (parser.prev.type) {
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    default: return;
  }
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,     NULL,   PREC_UNARY},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary,   PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,   PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,     NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static ParseRule* getRule(TokenType token) {
  return &rules[token];
}

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  compilingChunk = chunk;
  parser.hadError = false;
  parser.panicMode = false;
  advance();
  expression();
  consume(TOKEN_EOF, "Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}
