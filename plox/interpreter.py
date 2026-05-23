from expression_syntax_types import Visitor, Variable, This
import statement_syntax_types
import environment
from lox_tokens import TokenType
from lox_callable import LoxCallable, LoxFunction
from runtime_error import LoxRuntimeError
import time
from lox_class import LoxClass, LoxInstance
class NativeClockCall(LoxCallable):
  def __init__(self):
    super().__init__(0)

  def call(self, interpreter, args):
    return time.time()

class Interpreter(Visitor, statement_syntax_types.Visitor):
  def __init__(self, lox):
    self.environment = environment.Environment()
    self.globals = self.environment
    self.globals.define("clock", NativeClockCall)
    self.return_value = None
    self.locals = {}
    self.lox = lox

  def interpret(self, statements):
    try:
      for stmt in statements:
        stmt.accept(self)
    except LoxRuntimeError as error:
      self.lox.runtime_error(error)

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
    return self.lookup_variable(variable_expr)

  def visit_Grouping(self, grouping):
    return grouping.expr.accept(self)

  def visit_Assignment(self, assignment):
    new_val = assignment.value.accept(self)
    if assignment not in self.locals:
      self.globals.assign(assignment.name, new_val)
    else:
      dist = self.locals[assignment]
      self.environment.assign_at(dist, assignment.name, new_val)
    return new_val

  def visit_ExprStmt(self, expr_stmt):
    expr_stmt.expr.accept(self)

  def visit_PrintStmt(self, print_stmt):
    value = print_stmt.expr.accept(self)
    is_fn_or_class = isinstance(value, LoxClass) or isinstance(value, LoxFunction)
    print(value if not is_fn_or_class else value.to_string())

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
        if self.return_value is not None:
          return self.return_value
      finally:
        self.environment = prev

  def resolve(self, expr, depth):
    self.locals[expr] = depth

  def lookup_variable(self, var):
    if var not in self.locals:
      return self.globals.get(var.name)
    if isinstance(var, This):
      return self.environment.get_at(self.locals[var], "this")
    return self.environment.get_at(self.locals[var], var.name.lexeme)

  def visit_IfStmt(self, if_stmt):
    condition_result = if_stmt.condition.accept(self)
    if not condition_result and if_stmt.else_ is not None:
      return if_stmt.else_.accept(self)
    if condition_result: if_stmt.then.accept(self)

  def visit_Logical(self, logical):
    match logical.op.type:
      case TokenType.OR:
        left_result = logical.left_expr.accept(self)
        if left_result: return left_result
        right_result = logical.right_expr.accept(self)
        return right_result
      case TokenType.AND:
        left_result = logical.left_expr.accept(self)
        if not left_result: return left_result
        right_result = logical.right_expr.accept(self)
        return left_result and right_result
      case _:
        raise LoxRuntimeError(logical.op, "Unhandled logical op")

  def visit_WhileStmt(self, while_stmt):
    while while_stmt.condition.accept(self):
      while_stmt.body.accept(self)

  def visit_Call(self, call):
    callee = call.callee.accept(self)
    if not isinstance(callee, LoxCallable):
      raise LoxRuntimeError(call.closing_paren, "Can only call functions and classes")
    args_interpreted = [arg.accept(self) for arg in call.arguments]
    return callee.call(self, args_interpreted)

  def visit_Function(self, function):
    self.environment.define(function.name.lexeme, LoxFunction(function, self.environment, False))

  def visit_ReturnStmt(self, return_stmt):
    self.return_value = return_stmt.value.accept(self) if return_stmt.value is not None else None

  def visit_ClassStmt(self, class_stmt):
    methods = {}
    for fn in class_stmt.methods:
      method = LoxFunction(fn, environment, fn.name.lexeme == "init")
      methods[fn.name.lexeme] = method
    lox_class = LoxClass(class_stmt.name.lexeme, methods)
    self.environment.define(class_stmt.name.lexeme, lox_class)

  def visit_Get(self, get):
    object = get.object.accept(self)
    if isinstance(object, LoxInstance):
      return object.get(get.name)
    raise self.lox.error_with_token(get.name, "Only instances have properties")

  def visit_Set(self, set):
    object = set.object.accept(self)
    if not isinstance(object, LoxInstance):
      raise self.lox.error_with_token(set.name, "Only instances have properties")
    value = set.value.accept(self)
    object.set(set.name, value)
    return value

  def visit_This(self, this):
    return self.lookup_variable(this)
