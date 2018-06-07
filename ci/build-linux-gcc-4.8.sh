#!/bin/bash
set -x

examples=(bootstrap tasks systemc factory)

build_root=build_linux_gcc-4.8
function compile {
	mkdir $build_root
	cd $build_root
	cmake .. -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=g++-4.8 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -msse4.2 -Wno-deprecated-declarations -DENABLE_COOL_BASH"
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