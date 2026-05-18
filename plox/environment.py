from runtime_error import LoxRuntimeError
class Environment:
  def __init__(self, enclosing=None):
    self.values = []
    self.enclosing = enclosing

  def define(self, name, value):
    self.values.append((name, value))

  def get(self, name_to_find):
    for name, value in self.values:
      if name == name_to_find:
        return value
    if self.enclosing is not None: return self.enclosing.get(name_to_find)
    raise LoxRuntimeError(name_to_find, f"Undefined variable {name_to_find.lexeme}")

  def assign(self, name_var, new_val):
    for idx, (name, value) in enumerate(self.values):
      if name == name_var:
        self.values[idx] = (name, new_val)
        return
    if self.enclosing is not None: return self.enclosing.assign(name_var, new_val)
    raise LoxRuntimeError(name, f"Undefiend variable {name.lexeme}")
