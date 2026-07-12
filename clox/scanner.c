#include <stdio.h>
#include <string.h>
#include "scanner.h"
#include "common.h"

typedef struct {
  const char* start;
  const char* current;
  int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

bool isAtEnd() {
  return *scanner.current == '\0';
}

Token makeToken(TokenType type) {
  return (Token){
    .type = type,
    .start = scanner.start,
    .length = scanner.current - scanner.start,
    .line = scanner.line
  };
}

Token errorToken(const char* msg) {
  return (Token){
    .type = TOKEN_ERROR,
    .start = msg,
    .length = strlen(msg),
    .line = scanner.line
  };
}

char advance() {
  return *(scanner.current++);
}

bool match(char c) {
  if (isAtEnd()) return false;
  char current = *scanner.current;
  scanner.current++;
  return c == current;
}

char peek() {
  return *scanner.current;
}

char peekNext() {
  if (isAtEnd()) return '\0';
  return *(scanner.current + 1);
}

bool isComment() {
  if (peek() == '/' && peekNext() == '/') {
    while (peek() != '\n' && !isAtEnd()) advance();
    return true;
  }
  return false;
}

void skipWhitespace() {
  char c = peek();
  for (;
    (c = peek(),
    c == ' '  ||
    c == '\t' ||
    c == '\r' ||
    c == '\n' ||
    isComment());) {
    if (peek() == '\n') {
      scanner.line ++;
    }
    advance();
  }
}

Token string() {
  while (peek() != '"' &&
    !isAtEnd()) {
    if (peek() == '\n') ++scanner.line;
    advance();
  }
  if (isAtEnd()) return errorToken("Unterminated string.");
  return makeToken(TOKEN_STRING);
}

bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') || c == '_';
}

Token number() {
  while (isDigit(peek()) && !isAtEnd()) advance();
  if (peek() == '.' && isDigit(peekNext())) {
    advance();
    while (isDigit(peek()) && !isAtEnd()) advance();
  }
  return makeToken(TOKEN_NUMBER);
}

TokenType identifierType() {
  int length = scanner.current - scanner.start;
  switch (*scanner.start) {
    case 'a':
      if (length == 3 &&
        memcmp(scanner.start, "and", 3) == 0) return TOKEN_AND;
      break;
    case 'c':
      if (length == 5 &&
        memcmp(scanner.start, "class", 5) == 0) return TOKEN_CLASS;
      break;
    case 'e':
      if (length == 4 &&
        memcmp(scanner.start, "else", 4) == 0) return TOKEN_ELSE;
      break;
    case 'f':
      if (length == 5 &&
        memcmp(scanner.start, "false", 5) == 0) return TOKEN_FALSE;
      if (length == 3 &&
        memcmp(scanner.start, "for", 3) == 0) return TOKEN_FOR;
      if (length == 3 &&
        memcmp(scanner.start, "fun", 3) == 0) return TOKEN_FUN;
      break;
    case 'i':
      if (length == 2 &&
        memcmp(scanner.start, "if", 2) == 0) return TOKEN_IF;
      break;
    case 'n':
      if (length == 2 &&
        memcmp(scanner.start, "nil", 3) == 0) return TOKEN_NIL;
      break;
    case 'o':
      if (length == 2 &&
        memcmp(scanner.start, "or", 2) == 0) return TOKEN_OR;
      break;
    case 'p':
      if (length == 5 &&
        memcmp(scanner.start, "print", 5) == 0) return TOKEN_PRINT;
      break;
    case 'r':
      if (length == 6 &&
        memcmp(scanner.start, "return", 6) == 0) return TOKEN_RETURN;
      break;
    case 's':
      if (length == 5 &&
        memcmp(scanner.start, "super", 5) == 0) return TOKEN_SUPER;
      break;
    case 't':
      if (length == 4 &&
        memcmp(scanner.start, "this", 4) == 0) return TOKEN_THIS;
      if (length == 4 &&
        memcmp(scanner.start, "true", 4) == 0) return TOKEN_TRUE;
      break;
    case 'v':
      if (length == 3 &&
        memcmp(scanner.start, "var", 3) == 0) return TOKEN_VAR;
      break;
    case 'w':
      if (length == 5 &&
        memcmp(scanner.start, "while", 5) == 0) return TOKEN_WHILE;
      break;
  }
  return TOKEN_IDENTIFIER;
}

Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();
  return makeToken(identifierType());
}

Token scanToken() {
  skipWhitespace();
  scanner.start = scanner.current;
  if (isAtEnd()) { return makeToken(TOKEN_EOF); }
  char c = advance();
  if (isAlpha(c)) return identifier();
  if (isDigit(c)) return number();
  switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '.': return makeToken(TOKEN_DOT);
    case '-': return makeToken(TOKEN_MINUS);
    case '+': return makeToken(TOKEN_PLUS);
    case '/': return makeToken(TOKEN_SLASH);
    case '*': return makeToken(TOKEN_STAR);
    case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': return makeToken(match('=') ?
                TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>': return makeToken(match('=') ?
                      TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string();
  }
  return errorToken("Unexpected character.");
}
