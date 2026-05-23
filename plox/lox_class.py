from lox_callable import LoxCallable
from lox_instance import LoxInstance
class LoxClass(LoxCallable):
  def __init__(self, name, methods):
    super().__init__(0)
    self.name = name
    self.methods = methods

  def call(self, interpreter, args):
    instance = LoxInstance(self)
    return instance

  def to_string(self):
    return self.name
