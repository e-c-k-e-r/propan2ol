"""Builds the shared library with CMake (the repository root) and ships it inside the package; the Python code is ctypes over its C ABI,
so there is no extension module and the wheel is only platform-tagged."""
import os
import shutil
import subprocess
import sys
from setuptools import setup
from setuptools.command.build_py import build_py
from setuptools.dist import Distribution
from wheel.bdist_wheel import bdist_wheel

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class BuildWithCMake(build_py):
    def run(self):
        build = os.path.join(ROOT, "build", "python-wheel")
        subprocess.check_call(["cmake", "-S", ROOT, "-B", build, "-DCMAKE_BUILD_TYPE=Release"])
        subprocess.check_call(["cmake", "--build", build, "--target", "propan2ol_shared", "--config", "Release", "-j", "2"])
        lib = None
        for dirpath, _, files in os.walk(build):
            for f in files:
                if f in ("libpropan2ol.so", "libpropan2ol.dylib", "propan2ol.dll", "cygpropan2ol.dll"):
                    lib = os.path.join(dirpath, f)
        if lib is None:
            raise RuntimeError("the shared library was not built")
        super().run()
        shutil.copy(lib, os.path.join(self.build_lib, "propan2ol", os.path.basename(lib)))


class PlatformWheel(bdist_wheel):
    def get_tag(self):
        _, _, platform = super().get_tag()
        return "py3", "none", platform   # ctypes over a plain shared library: any Python 3, only the platform matters


class BinaryDistribution(Distribution):
    def has_ext_modules(self):
        return True   # a platform wheel, though there are no extension modules


setup(cmdclass={"build_py": BuildWithCMake, "bdist_wheel": PlatformWheel}, distclass=BinaryDistribution)
