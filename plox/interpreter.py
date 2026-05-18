from expression_syntax_types import Visitor, Unary, Binary, Literal, Grouping
import statement_syntax_types
import environment
from lox_tokens import TokenType
import lox
from runtime_error import LoxRuntimeError
class Interpreter(Visitor, statement_syntax_types.Visitor):
  def __init__(self):
    self.environment = environment.Environment()

  def interpret(self, statements):
    try:
      for stmt in statements:
        stmt.accept(self)
    except LoxRuntimeError as error:
      lox.runtime_error(error)

  def both_numbers(self, a, b): return isinstance(a, float) and isinstance(b, float)

  def is_number(self, a): return isinstance(a, float)

  def visit_Literal(self, literal):
    return literal.token.literal

  def visit_Binary(self, binary):
    left = binary.left_expr.accept(self)
    right = binary.right_expr.accept(self)
    match binary.op.type:
      case TokenType.LESS:
        if not self.both_numbers(left, right):
          raise LoxRuntimeError(binary.op, "operands weren't both numbers")
        return left < right
      case TokenType.LESS_EQUAL:
        if not self.both_numbers(left, right):
          raise LoxRuntimeError(binary.op, "operands weren't both numbers")
        return left <= right
      case TokenType.GREATER:
        if not self.both_numbers(left, right):
          raise LoxRuntimeError(binary.op, "operands weren't both numbers")
        return left > right
      case TokenType.GREATER_EQUAL:
        if not self.both_numbers(left, right):
          raise LoxRuntimeError(binary.op, "operands weren't both numbers")
        return left >= right
      case TokenType.BANG_EQUAL:
        return left != right
      case TokenType.EQUAL_EQUAL:
        return left == right
      case TokenType.MINUS:
        if not self.both_numbers(left, right):
          raise LoxRuntimeError(binary.op, "operands weren't both numbers")
        return left - right
      case TokenType.PLUS:
        if not self.both_numbers(left, right) and not isinstance(left, str) and not isinstance(right, str):
          raise LoxRuntimeError(binary.op, "not both numbers or not both strings")
        return left + right
      case TokenType.STAR:
        if not self.both_numbers(left, right):
          raise LoxRuntimeError(binary.op, "operands not both numbers")
        return left * right
      case TokenType.SLASH:
        if not self.both_numbers(left, right):
          raise LoxRuntimeError(binary.op, "operands not both numbers")
        return left / right
      case _: raise LoxRuntimeError(binary.op, f"unhandled binary op: {binary.op.type.name}")

  def visit_Unary(self, unary):
    result = unary.expr.accept(self)
    match unary.op.type:
      case TokenType.BANG:
        return not result
      case TokenType.MINUS:
        if not is_number(result):
          raise LoxRuntimeError(binary.op, "tried - on a non-number")
        return -result
      case _: raise LoxRuntimeError(binary.op, f"unhandled unary op: {unary.op.type.name}")

  def visit_Variable(self, variable_expr):
    return self.environment.get(variable_expr.name.lexeme)

  def visit_Grouping(self, grouping):
    return grouping.expr.accept(self)

  def visit_Assignment(self, assignment):
    new_val = assignment.value.accept(self)
    self.environment.assign(assignment.name.lexeme, new_val)
    return new_val

  def visit_ExprStmt(self, expr_stmt):
    expr_stmt.expr.accept(self)

  def visit_PrintStmt(self, print_stmt):
    value = print_stmt.expr.accept(self)
    print(value)

  def visit_Var(self, var_stmt):
    self.environment.define(var_stmt.name.lexeme, var_stmt.initializer.accept(self) if var_stmt.initializer is not None else None)

  def visit_Block(self, block):
    self.execute_block(block, self.environment)

  def execute_block(self, block, enclosing):
    prev = self.environment
    new_env = environment.Environment(enclosing)
    for stmt in block.lst_statements:
      self.environment = new_env
      try:
        stmt.accept(self)
      finally:
        self.environment = prev
