#!/bin/bash
set -x

examples=(bootstrap tasks systemc factory)

function compile {
	mkdir build_test
	cd build_test
	cmake .. -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=icpc -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -march=native -Wno-deprecated-declarations -std=c++11"
	THREADS=$(grep -c ^processor /proc/cpuinfo)
	make -j $THREADS
	cd ..
}

cd examples
for example in ${examples[*]}; do
	cd $example
	compile
	cd ..
done