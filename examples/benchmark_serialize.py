import timeit
import pickle
import taco


def f_native(x):
  if isinstance(x, list):
    return b"".join(map(f_native, x))
  elif isinstance(x, str):
    return x.encode()
  else:
    return pickle.dumps(x)


@taco.optimize
def f_v1(x):
  if isinstance(x, list):
    return taco.optimize(b"".join)(taco.map(f_v1, x))
  elif isinstance(x, str):
    return x.encode()
  else:
    return pickle.dumps(x)


join = taco.v2.optimize(b"".join)


@taco.v2.optimize
def f_v2(x):
  if isinstance(x, list):
    return join(taco.v2.map_pure(f_v2, x))
  elif isinstance(x, str):
    return x.encode()
  else:
    return pickle.dumps(x)


if __name__ == '__main__':
  number = 1
  data = [[None, ("asd", "pqw", 123, 123.4)], [[..., b"plw", 123, 96.7], {"A": 12}]]
  # print(f_v2(data).eval())
  print("native", timeit.timeit("f_native(data)", globals=globals(), number=number))
  print("v1", timeit.timeit("f_v1(data).eval()", globals=globals(), number=number))
  print("v2", timeit.timeit("f_v2(data).eval()", globals=globals(), number=number))
  print("pickle", timeit.timeit("pickle.dumps(data)", globals=globals(), number=number))
