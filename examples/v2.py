from taco.v2 import *


@optimize
def f(n, s=0):
  if n == 0:
    return s
  return f(n - 1, s + n)


@optimize
def f2(n):
  if n <= 1:
    return 1
  return add(f2(n-1), f2(n-2))


if __name__ == '__main__':
  print(evaluate(f(1000)))
  print(evaluate(map(f2, range(10))))
