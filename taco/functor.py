from .utils import *


@optimize
def fmap(f, cls, data):
  if isinstance(data, cls):
    return f(data)
  elif isinstance(data, list):
    return map(lambda x: fmap(f, cls, x), data)
  elif isinstance(data, dict):
    return optimize(dict)(map(lambda tup: eval(tup[0], fmap(f, cls, tup[1])), data.items()))

