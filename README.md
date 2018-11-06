# Using AFF3CT as a library for your codes

This repository contains some simple code examples. It helps to understand how to use the AFF3CT library in your code.
The first step is to compile AFF3CT into a library.

Get the AFF3CT library:

	$ git submodule update --init --recursive

Compile the code on Linux/MacOS/MinGW:

	$ cd lib/aff3ct
	$ mkdir build
	$ cd build
	$ cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-funroll-loops -march=native" -DAFF3CT_COMPILE_EXE="OFF" -DAFF3CT_COMPILE_STATIC_LIB="ON" -DCMAKE_INSTALL_PREFIX="install"
	$ make -j4
	$ make install

Compile the code on Windows (Visual Studio project)

	$ cd lib/aff3ct
	$ mkdir build
	$ cd build
	$ cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="-D_CRT_SECURE_NO_DEPRECATE /EHsc /MP4" -DAFF3CT_COMPILE_EXE="OFF" -DAFF3CT_COMPILE_STATIC_LIB="ON" -DCMAKE_INSTALL_PREFIX="install"
	$ devenv /build Release aff3ct.sln
	$ devenv /build Release aff3ct.sln /project INSTALL

Now the AFF3CT library has been built in the `lib/aff3ct/build` folder and installed in the `lib/aff3ct/build/install` folder.

The source codes of the examples are in the `examples/` folder.
You can go in this folder to see the next steps.