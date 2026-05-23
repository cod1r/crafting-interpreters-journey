from lox_callable import LoxCallable
class LoxClass(LoxCallable):
  def __init__(self, name, methods, superclass):
    self.name = name
    self.methods = methods
    self.superclass = superclass
    init = self.find_method("init")
    super().__init__(init.arity if init is not None else 0)

  def find_method(self, name):
    if name in self.methods: return self.methods[name]
    if self.superclass is not None:
      return self.superclass.find_method(name)

  def call(self, interpreter, args):
    instance = LoxInstance(self)
    init = self.find_method("init")
    if init is not None:
      init.bind(instance).call(interpreter, args)
    return instance

  def to_string(self):
    return self.name

class LoxInstance:
  def __init__(self, class_type):
    self.class_type = class_type
    self.fields = {}

  def get(self, name):
    if name.lexeme in self.fields: return self.fields[name.lexeme]
    method = self.class_type.find_method(name.lexeme)
    if method is not None: return method.bind(self)
    raise RuntimeError(f"Undefined property {name.lexeme}")

  def set(self, name, value):
    self.fields[name.lexeme] = value

  def to_string(self):
    return self.class_type.to_string() + " instance"
