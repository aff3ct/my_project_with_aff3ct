# How to compile this example

Get the AFF3CT library:

    $ git submodule update --init --recursive

Compile the code on Linux/MacOS/MinGW:

    $ mkdir build
    $ cd build
    $ cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-funroll-loops -march=native"
    $ make -j4

Create the project on Windows (Visual Studio, MSVC)

    $ mkdir build
    $ cd build
    $ cmake .. -G"Visual Studio 14 2015 Win64" -DCMAKE_CXX_FLAGS="-DMULTI_PREC -D_SCL_SECURE_NO_WARNINGS /EHsc /arch:AVX2" -DCMAKE_BUILD_TYPE=Release

The source code of this mini project is in `src/main.cpp`.
The compiled binary is in `build/bin/my_project`.

Have fun :-)!
