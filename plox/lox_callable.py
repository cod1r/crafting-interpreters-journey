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
  def __init__(self, declaration):
    super().__init__(len(declaration.parameters))
    self.declaration = declaration

  def call(self, interpreter, arguments):
    environment_ = environment.Environment(interpreter.globals)
    for idx, arg in enumerate(arguments):
      environment_.define(self.declaration.parameters[idx].lexeme, arg)

    interpreter.execute_block(self.declaration.body, environment_)

  def to_string(self):
    return f"<fn {self.declaration.name.lexeme}>"
