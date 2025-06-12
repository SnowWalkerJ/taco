from taco.v1.simple import tail_call_optimize


@tail_call_optimize
def f(n, s=0):
  if n == 0:
    return s
  return f(n - 1, s + n)


if __name__ == '__main__':
  print(f(100).eval())
