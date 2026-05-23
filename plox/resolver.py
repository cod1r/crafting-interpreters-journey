import expression_syntax_types
import statement_syntax_types
import lox
from enum import Enum, auto

class FunctionType(Enum):
  FUNCTION = auto()
  NONE = auto()
  METHOD = auto()

class Resolver(expression_syntax_types.Visitor, statement_syntax_types.Visitor):
  def __init__(self, interpreter, lox):
    self.interpreter = interpreter
    self.scopes = []
    self.current_function = FunctionType.NONE
    self.lox = lox

  def visit_Block(self, block):
    self.begin_scope()
    self.resolve(block.lst_statements)
    self.end_scope()

  def begin_scope(self):
    self.scopes.append({})

  def end_scope(self):
    self.scopes.pop()

  def resolve(self, syntax):
    if type(syntax) == list:
      for stmt in syntax:
        stmt.accept(self)
    else:
      syntax.accept(self)

  def visit_Function(self, function):
    self.declare(function.name)
    self.define(function.name)
    self.resolve_function(function, FunctionType.FUNCTION)

  def resolve_function(self, function, function_type):
    prev = self.current_function
    self.current_function = function_type
    self.begin_scope()
    for param in function.parameters:
      self.declare(param)
      self.define(param)
    self.resolve(function.body)
    self.end_scope()
    self.current_function = prev

  def visit_Var(self, var_decl):
    self.declare(var_decl.name)
    if var_decl.initializer is not None: self.resolve(var_decl.initializer)
    self.define(var_decl.name)

  def declare(self, name):
    if len(self.scopes) == 0: return

    inner_most = self.scopes[-1]
    if name.lexeme in inner_most: self.lox.error_with_token(name, f"Already a variable named {name.lexeme} in this scope")
    inner_most[name.lexeme] = False

  def define(self, name):
    if len(self.scopes) == 0: return
    inner_most = self.scopes[-1]
    inner_most[name.lexeme] = True

  def visit_Variable(self, var):
    if len(self.scopes) > 0 and self.scopes[-1][var.name.lexeme] == False:
      self.lox.error_with_token(var.name, "Cannot read local variable in its own initializer")
    self.resolve_local(var)

  def resolve_local(self, var):
    for idx in range(len(self.scopes) - 1, -1, -1):
      if var.name.lexeme in self.scopes[idx]:
        self.interpreter.resolve(var, len(self.scopes) - 1 - idx)
        return

  def visit_Assignment(self, assignment):
    self.resolve(assignment.value)
    self.resolve_local(assignment)

  def visit_ExprStmt(self, expr_stmt):
    self.resolve(expr_stmt.expr)

  def visit_IfStmt(self, if_stmt):
    self.resolve(if_stmt.condition)
    self.resolve(if_stmt.then)
    if if_stmt.else_ is not None: self.resolve(if_stmt.else_)

  def visit_PrintStmt(self, print_stmt):
    self.resolve(print_stmt.expr)

  def visit_ReturnStmt(self, return_stmt):
    if self.current_function is FunctionType.NONE:
      self.lox.error_with_token(return_stmt.return_token, "can't return from top level code")
    if return_stmt.value is not None:
      self.resolve(return_stmt.value)

  def visit_WhileStmt(self, while_stmt):
    self.resolve(while_stmt.condition)
    self.resolve(while_stmt.body)

  def visit_Binary(self, binary):
    self.resolve(binary.left_expr)
    self.resolve(binary.right_expr)

  def visit_Call(self, call):
    self.resolve(call.callee)
    for arg in call.arguments:
      self.resolve(arg)

  def visit_Grouping(self, grouping):
    self.resolve(grouping)

  def visit_Literal(self, literal):
    return

  def visit_Logical(self, logical):
    self.resolve(logical.left_expr)
    self.resolve(logical.right_expr)

  def visit_Unary(self, unary):
    self.resolve(unary.expr)

  def visit_ClassStmt(self, class_stmt):
    self.declare(class_stmt.name)
    self.define(class_stmt.name)
    self.begin_scope()
    self.scopes[-1]["this"] = True
    for method in class_stmt.methods:
      self.resolve_function(method, FunctionType.METHOD)
    self.end_scope()

  def visit_Get(self, get):
    self.resolve(get.object)

  def visit_Set(self, set):
    self.resolve(set.object)
    self.resolve(set.value)

  def visit_This(self, this):
    self.resolve_local(this.token)
