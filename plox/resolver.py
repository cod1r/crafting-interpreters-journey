from expression_syntax_types import This, Visitor as ExprVisitor, SuperExpr
from statement_syntax_types import Visitor as StmtVisitor
import lox
from enum import Enum, auto

class FunctionType(Enum):
  FUNCTION = auto()
  NONE = auto()
  METHOD = auto()
  INIT = auto()

class ClassType(Enum):
  NONE = auto()
  CLASS = auto()
  SUBCLASS = auto()

class Resolver(ExprVisitor, StmtVisitor):
  def __init__(self, interpreter, lox):
    self.interpreter = interpreter
    self.scopes = []
    self.current_function = FunctionType.NONE
    self.lox = lox
    self.current_class = ClassType.NONE

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
    if len(self.scopes) > 0 and var.name.lexeme in self.scopes[-1] and self.scopes[-1][var.name.lexeme] == False:
      self.lox.error_with_token(var.name, "Cannot read local variable in its own initializer")
    self.resolve_local(var, var)

  def resolve_local(self, expr, var):
    var_str = None
    if isinstance(expr, This):
      var_str = "this"
    elif isinstance(expr, SuperExpr):
      var_str = "super"
    else:
      var_str = var.name.lexeme
    for idx in range(len(self.scopes) - 1, -1, -1):
      if var_str in self.scopes[idx]:
        self.interpreter.resolve(expr, len(self.scopes) - 1 - idx)
        return

  def visit_Assignment(self, assignment):
    self.resolve(assignment.value)
    self.resolve_local(assignment, assignment)

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
      if self.current_function is FunctionType.INIT:
        self.lox.error_with_token(return_stmt.return_token, "can't return value from init method")
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
    self.resolve(grouping.expr)

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
    prev = self.current_class
    self.current_class = ClassType.CLASS

    if class_stmt.superclass is not None and class_stmt.superclass.name.lexeme == class_stmt.name.lexeme:
      self.lox.error_with_token(class_stmt.superclass.name, "cannot inherit itself")
    if class_stmt.superclass is not None:
      self.current_class = ClassType.SUBCLASS
      self.resolve(class_stmt.superclass)
      self.begin_scope()
      self.scopes[-1]["super"] = True

    self.begin_scope()
    self.scopes[-1]["this"] = True
    for method in class_stmt.methods:
      self.resolve_function(method, FunctionType.METHOD if method.name.lexeme != "init" else FunctionType.INIT)
    self.end_scope()

    if class_stmt.superclass is not None:
      self.end_scope()

    self.current_class = prev

  def visit_Get(self, get):
    self.resolve(get.object)

  def visit_Set(self, set):
    self.resolve(set.object)
    self.resolve(set.value)

  def visit_This(self, this):
    if self.current_class is ClassType.NONE:
      self.lox.error_with_token(this.token, "can't use 'this' in non class context")
      return
    self.resolve_local(this, None)

  def visit_SuperExpr(self, super_expr):
    if self.current_class is None:
      self.lox.error_with_token(super_expr.token, "Cannot use 'super' outside of a class")
    elif self.current_class is not ClassType.SUBCLASS:
      self.lox.error_with_token(super_expr.token, "Cannot use 'super' in a class without a superclass")
    self.resolve_local(super_expr, None)
