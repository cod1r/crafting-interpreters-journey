#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "common.h"
#include "scanner.h"
#include "chunk.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#define UINT8_COUNT (UINT8_MAX + 1)

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
  void (*prefix)(bool canAssign);
  void (*infix)(bool canAssign);
  Precedence prec;
} ParseRule;

typedef struct {
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;

Parser parser;
Compiler* currentCompiler = NULL;
Chunk* compilingChunk;

static void initCompiler(Compiler* compiler) {
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  currentCompiler = compiler;
}

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

static bool match(TokenType type) {
  if (parser.curr.type == type) { advance(); return true; }
  return false;
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
  void (*prefixRule)(bool) = getRule(parser.prev.type)->prefix;
  if (prefixRule == NULL) {
    error("Expected expression.");
    return;
  }
  bool canAssign = p <= PREC_ASSIGNMENT;
  prefixRule(canAssign);
  while (p <= getRule(parser.curr.type)->prec) {
    advance();
    void (*infixRule)(bool) = getRule(parser.prev.type)->infix;
    infixRule(canAssign);
  }
  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target");
  }
}

static uint8_t identifierConstant(Token* identifier) {
  return makeConstant(object_value((Obj*)copyString(identifier->start,
                                              identifier->length)));
}

static void addLocal(Token name) {
  if (currentCompiler->localCount > UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }
  Local* local = &currentCompiler->locals[currentCompiler->localCount++];
  local->name = name;
  local->depth = -1;
}

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static void declareVariable() {
  Token* name = &parser.prev;
  for (int i = currentCompiler->localCount - 1; i >= 0; i--) {
    Local* local = &currentCompiler->locals[i];
    if (local->depth != -1 && local->depth < currentCompiler->scopeDepth) {
      break;
    }
    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in the scope.");
    }
  }
  addLocal(*name);
}

static void markInitialized() {
  currentCompiler->locals[currentCompiler->localCount - 1].depth =
    currentCompiler->scopeDepth;
}

static void defineVariable(uint8_t global_var_name_idx) {
  if (currentCompiler->scopeDepth > 0) {
    markInitialized();
    return;
  }
  emitOpCodeOperand(OP_DEFINE_GLOBAL, global_var_name_idx);
}

static uint8_t parseVariable(const char* errorMsg) {
  consume(TOKEN_IDENTIFIER, errorMsg);
  if (currentCompiler->scopeDepth > 0) {
    declareVariable();
    return 0; // DUMMY index bc there won't be a constant index
  }
  return identifierConstant(&parser.prev);
}

static void binary(bool canAssign) {
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

static void unary(bool canAssign) {
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

static void declaration();
static void statement();

static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
  Value v = convert_to_value();
  emitConstant(v);
}

static void literal(bool canAssign) {
  switch (parser.prev.type) {
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    default: return;
  }
}

static void string(bool canAssign) {
  emitConstant(object_value((Obj*)copyString(parser.prev.start + 1,
                            parser.prev.length - 2)));
}

static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(&local->name, name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }
  return -1;
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t set_op, get_op;
  int arg_idx = resolveLocal(currentCompiler, &name);
  if (arg_idx != -1) {
    set_op = OP_SET_LOCAL;
    get_op = OP_GET_LOCAL;
  } else {
    arg_idx =  identifierConstant(&name);
    set_op = OP_SET_GLOBAL;
    get_op = OP_GET_GLOBAL;
  }
  if (match(TOKEN_EQUAL) && canAssign) {
    expression();
    emitOpCodeOperand(set_op, (uint8_t)arg_idx);
  } else {
    emitOpCodeOperand(get_op, (uint8_t)arg_idx);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.prev, canAssign);
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
  [TOKEN_IDENTIFIER]    = {variable,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,     NULL,   PREC_NONE},
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

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';'");
  emitByte(OP_PRINT);
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after expression");
  emitByte(OP_POP);
}

static void varDeclaration() {
  uint8_t var = parseVariable("Expect variable name.");
  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expected ';' after var declaration");
  defineVariable(var);
}

static void synchronize() {
  parser.panicMode = false;
  while (parser.curr.type != TOKEN_EOF) {
    switch (parser.prev.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;
      default:;
    }
    advance();
  }
}

static void declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode) synchronize();
}

static void block() {
  while (parser.curr.type != (TOKEN_RIGHT_BRACE) && parser.curr.type != (TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

static void beginScope() {
  currentCompiler->scopeDepth++;
}

static void endScope() {
  currentCompiler->scopeDepth--;
  while (currentCompiler->localCount > 0 &&
        currentCompiler->locals[currentCompiler->localCount - 1].depth >
        currentCompiler->scopeDepth) {
    emitByte(OP_POP);
    currentCompiler->localCount--;
  }
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler);
  compilingChunk = chunk;
  parser.hadError = false;
  parser.panicMode = false;
  advance();
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  endCompiler();
  return !parser.hadError;
}
