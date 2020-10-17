from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

codec = Extension(
    name="codec",
    sources=["codec.pyx"],
    libraries=["aff3ct-2.3.4"],
    library_dirs=["../../lib/aff3ct/build/lib"],
    include_dirs=["../../lib/aff3ct/src", "../../lib/aff3ct/lib/MIPP/src", "../../lib/aff3ct/lib/rang/include"],
    language="c++",
    extra_compile_args=["-std=c++14", "-Wno-overloaded-virtual", "-fopenmp"],
    extra_link_args=["-std=c++14", "-fopenmp"]
)
setup(
    name="codec",
    ext_modules=cythonize([codec])
)
