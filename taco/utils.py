import builtins
import functools
import operator
from .machine import *


__all__ = [
  "optimize",
  "map", "map_pure",
  "pack",
  "unpack",
  "abs",
  "add", "sub", "mul", "truediv", "floordiv", "mod", "pow",
  "eq", "ne", "ge", "gt", "le", "lt",
  "OptimizedCall",
  "MultiReturnOptimizedCall",
]


class OptimizedCall:
  def __init__(self, func):
    self.func = func

  def __call__(self, *args):
    return CallProducer(self.func, args)


class MultiReturnOptimizedCall:
  def __init__(self, func, num_returns):
    self.func = func
    self.num_returns = num_returns

  def __call__(self, *args):
    return MultiCallProducer(self.func, list(args), self.num_returns)


class DebugMachine:
  def __init__(self, data):
    self.left = data
    self.right = []

  def run(self):
    while self.left:
      item = self.left.pop(-1)
      if isinstance(item, Consumer):
        nargs = item.nargs()
        arguments = [self.right.pop(-1) for _ in range(nargs)]
        results = item.consume(arguments)
        self.left.extend(results)
      else:
        self.right.append(item)
    return self.right


def optimize(func=None, *, num_returns=None):
  if func is None:
    return functools.partial(optimize, num_returns=num_returns)
  if num_returns is None:
    return OptimizedCall(func)
  else:
    return MultiReturnOptimizedCall(func, num_returns)


def pack(producer):
  return PackProducer(producer)


def unpack(producer):
  return UnpackProducer(producer)


abs = optimize(builtins.abs)
add = optimize(operator.add)
sub = optimize(operator.sub)
mul = optimize(operator.mul)
truediv = optimize(operator.truediv)
floordiv = optimize(operator.floordiv)
mod = optimize(operator.mod)
pow = optimize(operator.pow)
eq = optimize(operator.eq)
ne = optimize(operator.ne)
ge = optimize(operator.ge)
gt = optimize(operator.gt)
lt = optimize(operator.lt)
le = optimize(operator.le)
