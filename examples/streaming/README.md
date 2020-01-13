# How to compile this example

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

The source code of this mini project is in `src/main.cpp`.
The compiled binary is in `build/bin/my_project`.

Example of command line for transferring a file with this example:

	$ ./bin/my_project -K 1023 -N 1023 --src-type USER_BIN --src-no-reset --chn-implem FAST --chn-type AWGN --snk-type USER_BIN --src-path <INPUT_FILE> --snk-path <OUTPUT_FILE>
