from abc import ABC
import environment
class LoxCallable(ABC):
  def __init__(self, arity):
    self.arity = arity

  def call(self, interpreter, args):
    pass

  def to_string(self):
    return "<native function>"

class LoxFunction(LoxCallable):
  def __init__(self, declaration, closure, is_init):
    super().__init__(len(declaration.parameters))
    self.declaration = declaration
    self.closure = closure
    self.is_init = is_init

  def bind(self, instance):
    env = environment.Environment(self.closure)
    env.define("this", instance)
    return LoxFunction(self.declaration, env, self.is_init)

  def call(self, interpreter, arguments):
    environment_ = environment.Environment(self.closure)
    for idx, arg in enumerate(arguments):
      environment_.define(self.declaration.parameters[idx].lexeme, arg)

    return_value = interpreter.execute_block(self.declaration.body, environment_)
    interpreter.return_value = None
    if self.is_init: return self.closure.get_at(0, "this")
    return return_value

  def to_string(self):
    return f"<fn {self.declaration.name.lexeme}>"
