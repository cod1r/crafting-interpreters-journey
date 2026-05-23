class LoxInstance:
  def __init__(self, class_type):
    self.class_type = class_type
    self.fields = {}

  def get(self, name):
    if name.lexeme in self.fields: return self.fields[name.lexeme]
    method = self.find_method(name)
    if method is not None: return method
    raise RuntimeError(f"Undefined property {name.lexeme}")

  def find_method(self, name):
    if name.lexeme in self.class_type.methods: return self.class_type.methods[name.lexeme]

  def set(self, name, value):
    self.fields[name.lexeme] = value

  def to_string(self):
    return self.class_type + " instance"
