from taco.v1.functor import *


if __name__ == '__main__':
  def inc(x):
    return x + 1

  print(fmap(inc, int, {"a": 1, "b": [1, 2]}).eval())