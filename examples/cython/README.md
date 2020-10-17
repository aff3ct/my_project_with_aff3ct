# How to compile this example as a standalone executable

Make sure to have done the instructions from the `README.md` file at the root of this repository before doing this.

Copy the cmake configuration files from the AFF3CT build

	$ mkdir cmake && mkdir cmake/Modules
	$ cp ../../lib/aff3ct/build/lib/cmake/aff3ct-*/* cmake/Modules

Compile the code on Linux/MacOS/MinGW:

	$ mkdir build
	$ cd build
	$ cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-funroll-loops -march=native"
	$ make

Compile the code on Windows (Visual Studio project)

	$ mkdir build
	$ cd build
	$ cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="-D_SCL_SECURE_NO_WARNINGS /EHsc"
	$ devenv /build Release my_project.sln

The source code of this mini project is in `src/cython.cpp`.
The compiled binary is in `build/bin/my_project`.

# How to compile this example as a Cython extension

Make sure you have followed the instructions from `README.md` file at the root of this repo before starting compiling.

Make sure you have installed the `Cython`, i.e.,

	$ pip install cython

Then build the Cython extension

	$ python setup.py build_ext -i

If you did not `make install` in the previous steps (i.e., building aff3ct), you will need to let the python know where to load the static library

	$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../../lib/aff3ct/build/lib

Then, test the cython module

	$ python -c "import codec; print(codec.py_generate_frozen_bits(10, 16, 20))"

If you see `ImportError: libaff3ct-2.3.4.so: cannot open shared object file: No such file or directory`, mostly the dynamic library is not set properly. Try `echo $LD_LIBRARY_PATH` and it should contains `libaff3ct-2.3.4.so`.

You can try a short demo script by

	$ python polar_codec.py