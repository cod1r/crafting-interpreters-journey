from lox_tokens import Token, TokenType
from syntax_types import Visitor, Binary, Unary, Literal, Grouping, Expression

class Ast_Printer(Visitor):
  def print(self, expr):
    return expr.accept(self)

  def visit_Binary(self, binary):
    return self.parenthesize(binary.op.lexeme, binary.left_expr, binary.right_expr)

  def visit_Unary(self, unary):
    return self.parenthesize(unary.op.lexeme, unary.expr)

  def visit_Grouping(self, grouping):
    return self.parenthesize("grouping", grouping.expr)

  def visit_Literal(self, literal):
    return str(literal.token.literal)

  def parenthesize(self, name, *exprs):
    s = "(" + name
    for expr in exprs:
      s += " " + expr.accept(self)
    s += ")"
    return s

expr = Binary(
          Unary(Token(TokenType.MINUS, "-", None, 1), Literal(Token(TokenType.NUMBER, "123", 123, 1))),
          Token(TokenType.STAR, "*", None, 1),
          Grouping(
            Literal(Token(TokenType.NUMBER, "45.67", 45.67, 1))
          )
        )

print(Ast_Printer().print(expr))
