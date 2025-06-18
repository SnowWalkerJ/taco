from glob import glob
from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext


sources = sorted(glob("taco/cpp/*.cpp"))
print(sources)

ext_modules = [
    Pybind11Extension(
        "machine",
        sources,  # Sort source files for reproducibility
    ),
]
import os
os.system("c++ -O3 -Wall -shared -std=c++11 -fPIC $(python3 -m pybind11 --includes) taco/cpp/machine.cpp -o taco/machine$(python3-config --extension-suffix) -std=c++20 -undefined dynamic_lookup")
# setup(name="taco", packages=["taco"], package_dir={"": "taco"}, cmdclass={"build_ext": build_ext}, ext_modules=ext_modules, script_args=["build_ext", "--inplace"])