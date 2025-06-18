#include <algorithm>
#include <stack>
#include <iterator>
#include <vector>
#include <memory>
#include <variant>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>


namespace py = pybind11;
using namespace pybind11::literals;

class Consumer;
using variant = std::variant<py::object, const Consumer *>;

class Consumer {
  public:
   virtual void consume(std::stack<variant> &stack_in, std::stack<variant> &stack_out) const = 0;
   virtual ~Consumer() {};
};


class PyConsumer: public Consumer {
 public:
  virtual size_t nargs() const = 0;
  virtual py::list py_consume(py::list args) const = 0;
  void consume(std::stack<variant> &stack_in, std::stack<variant> &stack_out) const override {
    size_t nargs = this->nargs();
    py::list arguments;
    for (size_t i = 0; i < nargs; i++) {
      auto item = std::move(stack_in.top());
      arguments.append(item.index() == 0 ? std::get<py::object>(item) : py::cast(std::get<const Consumer *>(item)));
      stack_in.pop();
    }
    auto result = py_consume(arguments).cast<std::vector<py::object>>();
    for (auto &&item : result) {
      auto var = py::isinstance<Consumer>(item) ? variant(item.cast<const Consumer *>()) : variant(std::move(item));
      stack_out.push(std::move(var));
    }
  }
};


class PyConsumerTrampoline : public PyConsumer {
 public:
  size_t nargs() const override {
    PYBIND11_OVERRIDE_PURE(
      size_t, /* Return type */
      Consumer,      /* Parent class */
      nargs,          /* Name of function in C++ (must match Python name) */
    );
  }
  py::list py_consume(py::list args) const override {
    PYBIND11_OVERRIDE_PURE(
      py::list, /* Return type */
      PyConsumer,      /* Parent class */
      py_consume,          /* Name of function in C++ (must match Python name) */
      args
    );
  }
};


class Producer : public Consumer {
 public:
  virtual void produce(std::stack<variant> &stack_out) const = 0;
  virtual py::object eval() const;
  void consume(std::stack<variant> &stack_in, std::stack<variant> &stack_out) const override {
    produce(stack_out);
  }
  virtual ~Producer() {};
};


class PyProducer : public Producer {
 public:
  virtual std::vector<py::object> py_produce() const = 0;
  void produce(std::stack<variant> &stack_out) const override {
    auto result = py_produce();
    for (auto &&item : result) {
      auto var = py::isinstance<Consumer>(item) ? variant(item.cast<const Consumer *>()) : variant(std::move(item));
      stack_out.push(std::move(var));
    }
  }
};


class PackConsumer : public Consumer {
 public:
  PackConsumer(size_t nargs) : nargs_(nargs) {}
  void consume(std::stack<variant> &stack_in, std::stack<variant> &stack_out) const override {
    py::list result;
    for (size_t i = 0; i < nargs_; i++) {
      result.append(std::get<py::object>(std::move(stack_in.top())));
      stack_in.pop();
    }
    stack_out.push(result);
  }
 private:
  size_t nargs_;
};


class PackProducer : public Producer {
 public:
  PackProducer(py::object producer) : producer_(producer) {}
  void produce(std::stack<variant> &stack_out) const override {
    auto producer = producer_.cast<Producer *>();
    stack_out.push(variant(new PackConsumer(py::len(producer_))));
    producer->produce(stack_out);
  }
  size_t size() const { return 1; }
 private:
  const py::object producer_;
};


class UnpackConsumer : public Consumer {
 public:
  UnpackConsumer() {}
  void consume(std::stack<variant> &stack_in, std::stack<variant> &stack_out) const override {
    auto item = std::get<py::object>(stack_in.top());
    stack_in.pop();
    auto list = item.cast<std::vector<py::object>>();
    for (auto &&elem : list) {
      stack_out.push(variant(std::move(elem)));
    }
  }
};


class UnpackProducer : public Producer {
 public:
  UnpackProducer(py::object producer) : producer_(producer) {}
  void produce(std::stack<variant> &stack_out) const override {
    auto producer = producer_.cast<Producer *>();
    stack_out.push(variant(new UnpackConsumer()));
    producer->produce(stack_out);
  }
 private:
  const py::object producer_;
};


class IntermediateCallProducer : public Producer {
 public:
  IntermediateCallProducer(py::object func, py::tuple args) : func_(func), args_(args) {}
  void produce(std::stack<variant> &stack_out) const override {
    return stack_out.push(variant(func_(*args_)));
  }
 private:
  py::object func_;
  py::tuple args_;
};


class CallConsumer : public Consumer {
 public:
  CallConsumer(py::object func, size_t nargs) : func_(func), nargs_(nargs) {}
  void consume(std::stack<variant> &stack_in, std::stack<variant> &stack_out) const override {
    py::list args;
    for (auto i = 0ul; i < nargs_; i++) {
      args.append(std::get<py::object>(std::move(stack_in.top())));
      stack_in.pop();
    }
    return stack_out.push(variant(func_(*args)));
  }
 private:
  py::object func_;
  size_t nargs_;
};


class CallProducer : public Producer {
 public:
  CallProducer(py::object func, std::vector<py::object> args) : func_(func), args_(std::move(args)) {}
  void produce(std::stack<variant> &stack_out) const override {
    stack_out.push(variant(new CallConsumer(func_, args_.size())));
    for (auto &&item : args_) {
      stack_out.push(item);
    }
  }
 private:
  py::object func_;
  std::vector<py::object> args_;
};


class MultiCallConsumer : public Consumer {
 public:
  MultiCallConsumer(py::object func, size_t num_returns, size_t nargs) : func_(func), num_returns_(num_returns), nargs_(nargs) {}
  void consume(std::stack<variant> &stack_in, std::stack<variant> &stack_out) const override {
    py::list args;
    for (auto i = 0ul; i < nargs_; i++) {
      args.append(std::get<py::object>(std::move(stack_in.top())));
      stack_in.pop();
    }
    py::object result = func_(*args);
    assert(py::len(result) == num_returns_);
    for (auto &&item : result.cast<std::vector<py::object> >()) {
      stack_out.push(variant(std::move(item)));
    }
  }
 private:
  py::object func_;
  std::vector<py::object> args_;
  size_t num_returns_;
  size_t nargs_;
};


class MultiCallProducer : public Producer {
 public:
  MultiCallProducer(py::object func, std::vector<py::object> args, size_t num_returns) : func_(func), args_(std::move(args)), num_returns_(num_returns) {}
    void produce(std::stack<variant> &stack_out) const override {
    stack_out.push(variant(new MultiCallConsumer(func_, num_returns_, args_.size())));
    for (auto &&item : args_) {
      stack_out.push(variant(item));
    }
  }
  size_t size() const { return num_returns_; }
 private:
  py::object func_;
  std::vector<py::object> args_;
  size_t num_returns_;
};


class ComboProducer : public Producer {
 public:
  explicit ComboProducer(std::vector<variant> objects) : objects_(std::move(objects)) {}
  void produce(std::stack<variant> &stack_out) const override {
    for (auto &&item : objects_) {
      stack_out.push(item);
    }
  }
 private:
  std::vector<variant> objects_;
};


class MapProducer : public Producer {
 public:
  MapProducer(py::object func, std::vector<py::object> args) : func_(func), args_(std::move(args)) {}
  void produce(std::stack<variant> &stack_out) const override {
    for (auto &&arg : args_) {
      stack_out.push(variant(new CallConsumer(func_, 1ul)));
      stack_out.push(arg);
    }
  }
  size_t size() const { return args_.size(); }
 private:
  py::object func_;
  std::vector<py::object> args_;
};


class Machine {
 public:
  Machine() = delete;
  Machine(const Consumer *initial) : initial_(initial) {
    left_.push(variant(initial));
  }
  std::vector<py::object> Run() {
    size_t num = 0, total_num = 0;
    while (!left_.empty()) {
      auto item = std::move(left_.top());
      left_.pop();
      if (item.index() == 1) {
        num++;
        auto consumer = std::get<const Consumer *>(item);
        consumer->consume(right_, left_);
        if (consumer != initial_) {
          delete consumer;
        }
      } else {
        py::object py_item = std::get<py::object>(item);
        if (py::isinstance<Consumer>(py_item)) {
          auto consumer = py_item.cast<const Consumer *>();
          consumer->consume(right_, left_);
          num++;
        } else {
          right_.push(item);
        }
      }
      total_num++;
    }
    std::vector<py::object> result;
    while (!right_.empty()) {
      result.push_back(std::get<py::object>(std::move(right_.top())));
      right_.pop();
    }
    py::print(num, "/", total_num);
    return result;
  }
 private:
  const Consumer *const initial_;
  std::stack<variant> left_, right_;
};


ComboProducer map(py::object func, const std::vector<py::object> &args) {
  py::module taco = py::module::import("taco");
  if (py::isinstance(func, taco.attr("OptimizedCall"))) {
    func = func.attr("func");
  }
  std::vector<variant> objects;
  objects.reserve(1 + args.size() * 2);
  objects.push_back(variant(new PackConsumer(args.size())));
  for (auto &&arg : args) {
    objects.push_back(variant(new CallConsumer(func, 1)));
    objects.push_back(std::move(arg));
  }
  return ComboProducer(std::move(objects));
}


ComboProducer map_pure(py::object func, const std::vector<py::object> &args) {
  py::module taco = py::module::import("taco");
  if (py::isinstance(func, taco.attr("OptimizedCall"))) {
    func = func.attr("func");
  }
  std::vector<variant> objects;
  objects.reserve(1 + args.size());
  objects.push_back(variant(new PackConsumer(args.size())));
  for (auto &&arg : args) {
    objects.push_back(variant(new IntermediateCallProducer(func, py::make_tuple(arg))));
  }
  return ComboProducer(std::move(objects));
}


py::object Producer::eval() const {
  return py::cast(Machine{this}.Run());
}


PYBIND11_MODULE(machine, m) {
//    py::class_<Machine>(m, "Machine")
//        .def(py::init<>())
//        .def(py::init<std::vector<py::object>>())
//        .def("run", &Machine::Run);

    py::class_<Consumer>(m, "Consumer");
    py::class_<PyConsumer, Consumer, PyConsumerTrampoline>(m, "PyConsumer")
        .def("consume", &PyConsumer::py_consume)
        .def("nargs", &PyConsumer::nargs);
    py::class_<Producer, Consumer>(m, "Producer")
        .def("produce", &Producer::produce)
        .def("eval", &Producer::eval);

    py::class_<CallConsumer, Consumer>(m, "CallConsumer")
        .def(py::init<py::object, size_t>(), "func"_a, "size"_a);
    py::class_<CallProducer, Producer>(m, "CallProducer")
        .def(py::init<py::object, std::vector<py::object>>(), "func"_a, "args"_a);
    py::class_<IntermediateCallProducer, Producer>(m, "IntermediateCallProducer")
        .def(py::init<py::object, py::tuple>(), "func"_a, "args"_a);
    py::class_<MultiCallConsumer, Consumer>(m, "MultiCallConsumer")
        .def(py::init<py::object, size_t, size_t>(), "func"_a, "num_returns"_a, "size"_a);
    py::class_<MultiCallProducer, Producer>(m, "MultiCallProducer")
        .def(py::init<py::object, std::vector<py::object>, size_t>(), "func"_a, "args"_a, "num_returns"_a)
        .def("__len__", &MultiCallProducer::size);
    py::class_<MapProducer, Producer>(m, "MapProducer")
        .def(py::init<py::object, std::vector<py::object>>(), "producer"_a, "args"_a)
        .def("__len__", &MapProducer::size);
//    py::class_<PackProducer, Producer>(m, "PackProducer")
//        .def(py::init<py::object>(), "producer"_a);
    py::class_<PackConsumer, Consumer>(m, "PackConsumer")
        .def(py::init<size_t>(), "nargs"_a);
    py::class_<UnpackProducer, Producer>(m, "UnpackProducer")
        .def(py::init<py::object>(), "producer"_a);
    py::class_<UnpackConsumer, Consumer>(m, "UnpackConsumer")
        .def(py::init<>());
    py::class_<ComboProducer, Producer>(m, "ComboProducer");
    m.def("map", &map, "func"_a, "args"_a);
    m.def("map_pure", &map_pure, "func"_a, "args"_a);
}
