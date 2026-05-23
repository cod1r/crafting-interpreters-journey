#! /usr/bin/env python3
import sys
import scanner
import parser
import ast_printer
import interpreter
import resolver
from lox_tokens import TokenType

class Lox:
  def __init__(self):
    self.had_error = False
    self.had_runtime_error = False

  def runtime_error(self, error):
    self.had_runtime_error = True
    print(error.message, f"Line: {error.token.line} at {error.token.lexeme}")

  def error(self, line, message):
    self.had_error = True
    self.report(line, "", message)

  def report(self, line, where, message):
    print('line', line, where, message)

  def error_with_token(self, token, message):
    self.had_error = True
    if token.type == TokenType.EOF:
      self.report(token.line, "", "at end " + message)
      return RuntimeError(f"{token.line} at end {message}")
    self.report(token.line, token.lexeme, message)
    return RuntimeError(f"{token.line} at '{token.lexeme} {message})")

  def run(self, file_content):
    scanner_ = scanner.Scanner(file_content, self)
    tokens = scanner_.scan_tokens()
    if self.had_error: return
    parser_ = parser.Parser(tokens, self)
    stmts = parser_.parse()
    if self.had_error: return
    interpreter_ = interpreter.Interpreter(self)
    resolver_ = resolver.Resolver(interpreter_, self)
    resolver_.resolve(stmts)
    if self.had_error: return
    interpreter_.interpret(stmts)

  def run_file(self, file_name):
    with open(file_name, "r+") as f:
      s = f.read()
    self.run(s)
    if self.had_error: exit(65)
    if self.had_runtime_error: exit(70)

  def run_repl(self):
    while line := input():
      self.run(line)
    self.had_error = False

if __name__ == "__main__":
  lox = Lox()
  if len(sys.argv) == 2:
    lox.run_file(sys.argv[1])
  elif len(sys.argv) > 2:
    print("Usage: plox [script]")
    exit(64)
  elif len(sys.argv) == 1:
    lox.run_repl()
