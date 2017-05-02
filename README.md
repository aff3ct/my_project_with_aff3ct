# How to compile this project

First, clone `aff3ct` into the `lib` folder:

    $ git clone git@gitlab.inria.fr:fec/aff3ct.git lib/aff3ct

And then, compile the code:

    $ mkdir build
    $ cd build
    $ cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-funroll-loops -march=native -DENABLE_COOL_BASH"
    $ make -j4

The source code of this mini project is in `src/main.cpp`.
The compiled binary is in `build/bin/my_project`.

Have fun :-)!
