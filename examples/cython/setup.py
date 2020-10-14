from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

aff3ct_extension = Extension(
    name="cython_example",
    sources=["cython_example.pyx"],
    libraries=["aff3ct-2.3.4"],
    library_dirs=["../../lib/aff3ct/build/lib"],
    include_dirs=["../../lib/aff3ct/src", "../../lib/aff3ct/lib/MIPP/src", "../../lib/aff3ct/lib/rang/include"],
    language="c++",
    extra_compile_args=["-std=c++11"],
    extra_link_args=["-std=c++11"]
)
setup(
    name="hello",
    ext_modules=cythonize([aff3ct_extension])
)