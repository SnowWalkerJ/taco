import abc


class Producer(abc.ABC):
  @abc.abstractmethod
  def produce(self) -> list:
    ...

  def eval(self):
    ...


class Consumer(abc.ABC):
  @abc.abstractmethod
  def nargs(self) -> int:
    ...

  @abc.abstractmethod
  def consume(self, args: list) -> list:
    ...


class CallProducer(Producer):
  def __init__(self, func, args):
    ...


class CallConsumer(Consumer):
  def __init__(self, func, nargs: int):
    ...


class MultiCallProducer(Producer):
  def __init__(self, funcs, args, num_returns: int):
    ...


class MultiCallConsumer(Consumer):
  def __init__(self, funcs, num_returns: int):
    ...


class MapProducer(Producer):
  def __init__(self, funcs, args):
    ...


class PackConsumer(Consumer):
  def __init__(self, nargs: int):
    ...


class PackProducer(Producer):
  def __init__(self, producer: Producer):
    ...


class UnpackProducer(Producer):
  def __init__(self, producer: Producer):
    ...


class ComboProducer(Producer):
  def __init__(self, objects: list):
    ...


class Machine:
  def __init__(self, objs: list):
    ...

  def run(self) -> list:
    ...


def map(func, args: list):
  ...


def map_pure(func, args: list):
  ...
