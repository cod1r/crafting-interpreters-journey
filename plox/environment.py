from runtime_error import LoxRuntimeError
class Environment:
  def __init__(self, enclosing=None):
    self.values = {}
    self.enclosing = enclosing

  def define(self, name, value):
    self.values[name] = value

  def get(self, name_to_find):
    if name_to_find.lexeme in self.values: return self.values[name_to_find.lexeme]
    if self.enclosing is not None: return self.enclosing.get(name_to_find)
    raise LoxRuntimeError(name_to_find, f"Undefined variable {name_to_find.lexeme}")

  def assign(self, name_var, new_val):
    if name_var.lexeme in self.values:
      self.values[name_var.lexeme] = new_val
      return
    if self.enclosing is not None: return self.enclosing.assign(name_var, new_val)
    raise LoxRuntimeError(name_var, f"Undefined variable {name_var.lexeme}")

  def get_at(self, dist, name_to_find):
    values = self.ancestor(dist).values
    return values[name_to_find]

  def ancestor(self, dist):
    env = self
    for _ in range(dist):
      env = env.enclosing
    return env

  def assign_at(self, dist, name_to_find, value):
    values = self.ancestor(dist).values
    values[name_to_find.lexeme] = value
