from lox_tokens import TokenType, Token
import lox

class Scanner:
  def __init__(self, src, lox):
    self.src = src
    self.tokens = []
    self.current = 0
    self.start = 0
    self.line = 1
    self.lox = lox

  def scan_tokens(self):
    while not self.is_at_end():
      self.start = self.current
      self.scan_token()
    self.tokens.append(Token(TokenType.EOF, "", None, self.line))
    return self.tokens
  
  def is_at_end(self):
    return self.current >= len(self.src)

  def scan_token(self):
    char = self.advance()
    match char:
      case '(':
        self.add_token(TokenType.LEFT_PAREN)
      case ')':
        self.add_token(TokenType.RIGHT_PAREN)
      case '{':
        self.add_token(TokenType.LEFT_BRACE)
      case '}':
        self.add_token(TokenType.RIGHT_BRACE)
      case ',':
        self.add_token(TokenType.COMMA)
      case '.':
        self.add_token(TokenType.DOT)
      case '-':
        self.add_token(TokenType.MINUS)
      case '+':
        self.add_token(TokenType.PLUS)
      case ';':
        self.add_token(TokenType.SEMICOLON)
      case '*':
        self.add_token(TokenType.STAR)
      case '!':
        self.add_token(TokenType.BANG_EQUAL if self.match('=') else TokenType.BANG)
      case '=':
          self.add_token(TokenType.EQUAL_EQUAL if self.match('=') else TokenType.EQUAL)
      case '<':
          self.add_token(TokenType.LESS_EQUAL if self.match('=') else TokenType.LESS)
      case '>':
          self.add_token(TokenType.GREATER_EQUAL if self.match('=') else TokenType.GREATER)
      case '/':
        if self.match('/'):
          while self.peek() != '\n' and not self.is_at_end():
            self.advance()
        else:
          self.add_token(TokenType.SLASH)
      case ' ' | '\r' | '\t':
        pass
      case '\n':
        self.line += 1
      case '"':
        self.string()
      case _:
        if char.isdigit():
          self.number()
        elif char.isalnum():
          self.identifier()
        else:
          self.lox.error(self.line, f"Unexpected character {char}")

  def add_token(self, tokentype, literal=None):
    text = self.src[self.start : self.current]
    self.tokens.append(Token(tokentype, text, literal, self.line))

  def advance(self):
    char = self.src[self.current]
    self.current += 1
    return char

  def match(self, char):
    if self.is_at_end(): return False
    if self.src[self.current] != char: return False
    self.current += 1
    return True

  def peek(self):
    if self.is_at_end(): return ''
    return self.src[self.current]

  def string(self):
    while not self.is_at_end() and self.peek() != '"':
      if self.peek() == '\n': self.line += 1
      self.advance()
    if self.is_at_end():
      self.lox.error(self.line, "Unterminated string")
      return
    self.advance()
    literal = self.src[self.start + 1 : self.current - 1]
    self.add_token(TokenType.STRING, literal)

  def number(self):
    while not self.is_at_end() and self.peek().isdigit():
      self.advance()
    if self.peek() == '.':
      self.advance()
      while not self.is_at_end() and self.peek().isdigit():
        self.advance()
    literal = self.src[self.start : self.current]
    self.add_token(TokenType.NUMBER, float(literal))

  def find_keyword(self, s):
    keywords = [
      ["and",    TokenType.AND],
      ["class",  TokenType.CLASS],
      ["else",   TokenType.ELSE],
      ["false",  TokenType.FALSE],
      ["for",    TokenType.FOR],
      ["fun",    TokenType.FUN],
      ["if",     TokenType.IF],
      ["nil",    TokenType.NIL],
      ["or",     TokenType.OR],
      ["print",  TokenType.PRINT],
      ["return", TokenType.RETURN],
      ["super",  TokenType.SUPER],
      ["this",   TokenType.THIS],
      ["true",   TokenType.TRUE],
      ["var",    TokenType.VAR],
      ["while",  TokenType.WHILE],
    ]
    for keyword, tokentype in keywords:
      if keyword == s.lower():
        return tokentype
    return TokenType.IDENTIFIER

  def identifier(self):
    while not self.is_at_end() and self.peek().isalnum():
      self.advance()
    literal = self.src[self.start : self.current]
    tokentype = self.find_keyword(literal)
    self.add_token(tokentype, literal)
