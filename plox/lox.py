#! /usr/bin/env python3
import sys
import scanner
import parser
import ast_printer
import interpreter
from lox_tokens import TokenType

interpreter_ = interpreter.Interpreter()

had_error = False
had_runtime_error = False

def runtime_error(error):
  had_runtime_error = True
  print(error.message, f"Line: {error.token.line}")

def error(line, message):
  had_error = True
  report(line, "", message)

def report(line, where, message):
  print(line, where, message)

def error_with_token(token, message):
  had_error = True
  if token.type == TokenType.EOF:
    report(token.line, " at end", message)
  else:
    report(token.line, "at '" + token.lexeme + "'", message)

def run(file_content):
  scanner_ = scanner.Scanner(file_content)
  tokens = scanner_.scan_tokens()
  parser_ = parser.Parser(tokens)
  stmts = parser_.parse()
  if had_error: return
  interpreter_.interpret(stmts)

def run_file(file_name):
  with open(file_name, "r+") as f:
    s = f.read()
  run(s)
  if had_error: exit(65)
  if had_runtime_error: exit(70)

def run_repl():
  while line := input():
    run(line)
  had_error = false

if __name__ == "__main__":
  if len(sys.argv) == 2:
    run_file(sys.argv[1])
  elif len(sys.argv) > 2:
    print("Usage: plox [script]")
    exit(64)
  elif len(sys.argv) == 1:
    run_repl()
