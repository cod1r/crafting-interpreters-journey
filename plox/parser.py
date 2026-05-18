from expression_syntax_types import Binary, Unary, Literal, Grouping, Variable, Assignment
from statement_syntax_types import PrintStmt, ExprStmt, Var, Block
from lox_tokens import TokenType
import lox
class Parser:
  def __init__(self, tokens):
    self.tokens = tokens
    self.current = 0

  def parse(self):
    stmts = []
    try:
      while not self.is_at_end():
        stmt = self.declaration()
        stmts.append(stmt)
      return stmts
    except RuntimeError as parse_error:
      self.synchronize()
      return None

  def declaration(self):
    if self.match(TokenType.VAR):
      return self.var_decl()
    return self.statement()

  def var_decl(self):
    var_token = None
    initializer = None
    if self.match(TokenType.IDENTIFIER):
      var_token = self.previous()
    else:
      lox.error(self.peek(), "expected identifier")
      raise RuntimeError("expected identifier")
    if self.match(TokenType.EQUAL):
      initializer = self.expression()
    if self.match(TokenType.SEMICOLON):
      return Var(var_token, initializer)
    lox.error(self.peek(), "expected ';' or assignment")
    raise RuntimeError("expected ';' or assignment")

  def statement(self):
    if self.match(TokenType.PRINT):
      return self.print_statement()
    if self.match(TokenType.LEFT_BRACE):
      return self.block()
    return self.expr_statement()

  def block(self):
    statements = []
    while not self.is_at_end() and not self.peek().type == TokenType.RIGHT_BRACE:
      statements.append(self.declaration())
    if self.match(TokenType.RIGHT_BRACE):
      return Block(statements)
    lox.error(self.previous().type, "Expected '}'")
    raise RuntimeError("Expected '}'")

  def expr_statement(self):
    expr = self.expression()
    if self.match(TokenType.SEMICOLON):
      return ExprStmt(expr)
    lox.error(self.peek(), "Expected ';'")
    raise RuntimeError("Expected ';'")

  def print_statement(self):
    expr = self.expression()
    if self.match(TokenType.SEMICOLON):
      return PrintStmt(expr)
    lox.error(self.peek(), "Expected ';'")
    raise RuntimeError("Expected ';'")

  def expression(self):
    return self.assignment()

  def assignment(self):
    expr = self.equality()
    if self.match(TokenType.EQUAL):
      eq = self.previous()
      value = self.assignment()
      if isinstance(expr, Variable):
        return Assignment(expr.name, value)
      lox.error(eq, "Invalid assignment")
    return expr

  def equality(self):
    expr = self.comparison()
    while self.match(TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL):
      operator = self.previous()
      right_expr = self.comparison()
      expr = Binary(expr, operator, right_expr)
    return expr

  def comparison(self):
    expr = self.term()
    while self.match(TokenType.GREATER, TokenType.GREATER_EQUAL, TokenType.LESS, TokenType.LESS_EQUAL):
      operator = self.previous()
      right_expr = self.term()
      expr = Binary(expr, operator, right_expr)
    return expr

  def term(self):
    expr = self.factor()
    while self.match(TokenType.MINUS, TokenType.PLUS):
      operator = self.previous()
      right_expr = self.factor()
      expr = Binary(expr, operator, right_expr)
    return expr

  def factor(self):
    expr = self.unary()
    while self.match(TokenType.SLASH, TokenType.STAR):
      operator = self.previous()
      right_expr = self.unary()
      expr = Binary(expr, operator, right_expr)
    return expr

  def unary(self):
    if self.match(TokenType.BANG, TokenType.MINUS):
      op = self.previous()
      expr = self.unary()
      return Unary(op, expr)
    return self.primary()

  def primary(self):
    if self.match(TokenType.NUMBER, TokenType.STRING, TokenType.TRUE, TokenType.FALSE, TokenType.NIL):
      return Literal(self.previous())
    if self.match(TokenType.LEFT_PAREN):
      expr = self.expression()
      if self.match(TokenType.RIGHT_PAREN):
        expr = Grouping(expr)
        return expr
      else:
        lox.error_with_token(self.previous(), "Expected ')'")
        raise RuntimeError("parse error")
    if self.match(TokenType.IDENTIFIER):
      return Variable(self.previous())
    lox.error_with_token(self.previous(), "Expected '(' or NUMBER,STRING,TRUE,FALSE,NIL")
    raise RuntimeError("parse error")

  def match(self, *args):
    if self.is_at_end(): return False
    if self.tokens[self.current].type in args:
      self.current += 1
      return True
    return False

  def previous(self):
    return self.tokens[self.current - 1]

  def is_at_end(self):
    return self.tokens[self.current].type == TokenType.EOF

  def peek(self):
    return self.tokens[self.current]

  def advance(self):
    if not self.is_at_end(): self.current += 1
    return self.previous()

  def synchronize(self):
    self.advance()
    while not self.is_at_end():
      if self.peek().type == TokenType.SEMICOLON: return
      match self.peek().type:
        case TokenType.CLASS | TokenType.FUN | TokenType.VAR | TokenType.FOR | TokenType.IF | TokenType.WHILE | TokenType.PRINT | TokenType.RETURN:
          return
        case _:
          pass
      self.advance()
