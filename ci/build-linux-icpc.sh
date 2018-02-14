#!/bin/bash
set -x

examples=(bootstrap tasks systemc factory)

function compile {
	mkdir build_test
	cd build_test
	cmake .. -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=icpc -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -march=native -Wno-deprecated-declarations  -DENABLE_COOL_BASH -std=c++11"
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	THREADS=$(grep -c ^processor /proc/cpuinfo)
	make -j $THREADS
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	cd ..
}

cd examples
for example in ${examples[*]}; do
	cd $example
	if [[ $example == systemc ]]; then
		mkdir cmake
		mkdir cmake/Modules
		cp $SYSTEMC_HOME/FindSystemC.cmake cmake/Modules/
		cp $SYSTEMC_HOME/FindTLM.cmake cmake/Modules/
	fi
	compile
	cd ..
done