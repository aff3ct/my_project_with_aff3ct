from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

codec = Extension(
    name="codec",
    sources=["codec.pyx"],
    libraries=["aff3ct-2.3.5-384-gec40f26"],
    library_dirs=["../../lib/aff3ct/build/lib"],
    include_dirs=["../../lib/aff3ct/include", "../../lib/aff3ct/lib/cli/src", "../../lib/aff3ct/lib/MIPP/src", "../../lib/aff3ct/lib/MIPP/src", "../../lib/aff3ct/lib/rang/include"],
    language="c++",
    extra_compile_args=["-std=c++14"],
    extra_link_args=["-std=c++14"]
)
setup(
    name="codec",
    ext_modules=cythonize([codec])
)
