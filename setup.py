from glob import glob
from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension

ext_modules = [
    Pybind11Extension(
        "machiine",
        sorted(glob("cpp/*.cpp")),  # Sort source files for reproducibility
    ),
]

setup(name="taco", package_dir={"": "taco"}, ext_modules=ext_modules, script_args=["build_ext", "--inplace"])