from expression_syntax_types import Binary, Unary, Literal, Grouping, Variable, Assignment, Logical, Call
from statement_syntax_types import PrintStmt, ExprStmt, Var, Block, IfStmt, WhileStmt, Function
from lox_tokens import TokenType, Token
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
    if self.match(TokenType.FUN):
      return self.fun("function")
    return self.statement()

  def fun(self, kind):
    if self.match(TokenType.IDENTIFIER):
      name = self.previous()
      if self.match(TokenType.LEFT_PAREN):
        parameters = self.parameters()
        if self.match(TokenType.LEFT_BRACE):
          body = self.block()
          return Function(name, parameters, body)
        raise lox.error_with_token(self.peek(), "Expected '{' for function body")
      raise lox.error_with_token(self.peek(), f"Expected '(' after {kind} name")
    raise lox.error_with_token(self.peek(), f"Expected {kind} name.")

  def parameters(self):
    parameters = []
    if self.match(TokenType.IDENTIFIER):
      parameters.append(self.previous())
    while self.match(TokenType.COMMA):
      if self.match(TokenType.IDENTIFIER):
        parameters.append(self.previous())
        if len(parameters) >= 255:
          lox.error_with_token(self.peek(), "cannot have 255+ parameters")
      else:
        raise lox.error_with_token(self.peek(), "Expected parameter name")
    if self.match(TokenType.RIGHT_PAREN):
      return parameters
    raise lox.error_with_token(self.peek(), "Expected ')'")

  def var_decl(self):
    var_token = None
    initializer = None
    if self.match(TokenType.IDENTIFIER):
      var_token = self.previous()
    else:
      raise lox.error_with_token(self.peek(), "expected identifier")
    if self.match(TokenType.EQUAL):
      initializer = self.expression()
    if self.match(TokenType.SEMICOLON):
      return Var(var_token, initializer)
    raise lox.error_with_token(self.previous(), "expected ';' or assignment")

  def statement(self):
    if self.match(TokenType.PRINT):
      return self.print_statement()
    if self.match(TokenType.LEFT_BRACE):
      return self.block()
    if self.match(TokenType.IF):
      return self.if_stmt()
    if self.match(TokenType.WHILE):
      return self.while_stmt()
    if self.match(TokenType.FOR):
      return self.for_stmt()
    return self.expr_statement()

  def while_stmt(self):
    if self.match(TokenType.LEFT_PAREN):
      condition = self.expression()
      if self.match(TokenType.RIGHT_PAREN):
        body = self.statement()
        return WhileStmt(condition, body)
      raise lox.error_with_token(self.peek(), "Expected ')'")
    raise lox.error_with_token(self.peek(), "Expected '('")

  def for_stmt(self):
    if self.match(TokenType.LEFT_PAREN):
      init_stmt = None
      if self.match(TokenType.VAR):
        init_stmt = self.var_decl()
      elif not self.match(TokenType.SEMICOLON):
        init_stmt = self.expr_statement()
      condition = None
      if not self.peek().type == TokenType.SEMICOLON:
        condition = self.expression()
      increment = None
      if self.match(TokenType.SEMICOLON):
        if self.peek().type != TokenType.RIGHT_PAREN:
          increment = self.expression()
        if self.match(TokenType.RIGHT_PAREN):
          body = self.statement()
          statements = []
          if init_stmt is not None:
            statements.append(init_stmt)
          statements.append(
            WhileStmt(
              Literal(
                Token(TokenType.TRUE, "true", True, None)) if condition is None else condition,
              Block([body] + ([] if increment is None else [ExprStmt(increment)]))
            )
          )
          return Block(statements)
        raise lox.error_with_token(self.peek(), "Expected ')'")
      raise lox.error_with_token(self.peek(), "Expected ';'")
    raise lox.error_with_token(self.peek(), "Expected '('")

  def if_stmt(self):
    if self.match(TokenType.LEFT_PAREN):
      condition = self.expression()
      if self.match(TokenType.RIGHT_PAREN):
        then = self.statement()
        else_ = None
        if self.match(TokenType.ELSE):
          else_ = self.statement()
        return IfStmt(condition, then, else_)
      else:
        raise lox.error_with_token(self.peek(), "Expected ')'")
    raise lox.error_with_token(self.peek(), "Expected 'if'")

  def block(self):
    statements = []
    while not self.is_at_end() and not self.peek().type == TokenType.RIGHT_BRACE:
      statements.append(self.declaration())
    if self.match(TokenType.RIGHT_BRACE):
      return Block(statements)
    raise lox.error_with_token(self.previous(), "Expected '}'")

  def expr_statement(self):
    expr = self.expression()
    if self.match(TokenType.SEMICOLON):
      return ExprStmt(expr)
    raise lox.error_with_token(self.peek(), "Expected ';'")

  def print_statement(self):
    expr = self.expression()
    if self.match(TokenType.SEMICOLON):
      return PrintStmt(expr)
    raise lox.error_with_token(self.peek(), "Expected ';'")

  def expression(self):
    return self.assignment()

  def assignment(self):
    expr = self.or_expr()
    if self.match(TokenType.EQUAL):
      eq = self.previous()
      value = self.assignment()
      if isinstance(expr, Variable):
        return Assignment(expr.name, value)
      lox.error_with_token(eq, "Invalid assignment")
    return expr

  def or_expr(self):
    expr = self.and_expr()
    while self.match(TokenType.OR):
      op = self.previous()
      right_expr = self.and_expr()
      expr = Logical(expr, op, right_expr)
    return expr

  def and_expr(self):
    expr = self.equality()
    while self.match(TokenType.AND):
      op = self.previous()
      right_expr = self.equality()
      expr = Logical(expr, op, right_expr)
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
    return self.call()

  def call(self):
    expr = self.primary()
    while self.match(TokenType.LEFT_PAREN):
      args = self.arguments()
      if self.match(TokenType.RIGHT_PAREN):
        expr = Call(expr, self.previous(), args)
      else:
        raise lox.error_with_token(self.peek(), "Expected ')'")
    return expr

  def arguments(self):
    lst = []
    if self.peek().type != TokenType.COMMA and self.peek().type != TokenType.RIGHT_PAREN:
      lst.append(self.expression())
    while self.match(TokenType.COMMA):
      lst.append(self.expression())
      if len(lst) >= 255:
        lox.error_with_token(self.peek(), "CANNOT HAVE 255+ arguments to a function")
    return lst

  def primary(self):
    if self.match(TokenType.NUMBER, TokenType.STRING, TokenType.TRUE, TokenType.FALSE, TokenType.NIL):
      return Literal(self.previous())
    if self.match(TokenType.LEFT_PAREN):
      expr = self.expression()
      if self.match(TokenType.RIGHT_PAREN):
        expr = Grouping(expr)
        return expr
      else:
        raise lox.error_with_token(self.previous(), "Expected ')'")
    if self.match(TokenType.IDENTIFIER):
      return Variable(self.previous())
    raise lox.error_with_token(self.peek(), "Expected '(' or NUMBER,STRING,TRUE,FALSE,NIL")

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
