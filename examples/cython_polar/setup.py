from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

def discover_library(path):
    import os
    return [os.path.splitext(f)[0] for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))][0][3:]

library = discover_library("../../lib/aff3ct/build/lib")

codec_polar = Extension(
    name="codec_polar",
    sources=["codec_polar.pyx"],
    libraries=[library],
    library_dirs=["../../lib/aff3ct/build/lib"],
    include_dirs=["../../lib/aff3ct/include", "../../lib/aff3ct/lib/cli/src", "../../lib/aff3ct/lib/MIPP/src", "../../lib/aff3ct/lib/MIPP/src", "../../lib/aff3ct/lib/rang/include"],
    language="c++",
    extra_compile_args=["-std=c++11"],
    extra_link_args=["-std=c++11"]
)
setup(
    name="codec_polar",
    ext_modules=cythonize([codec_polar])
)
