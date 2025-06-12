from taco import *


@optimize
def f(n):
  if n <= 1:
    return 1
  return add(f(n-1), f(n-2))


if __name__ == '__main__':
  print(map(f, range(10)).eval())
