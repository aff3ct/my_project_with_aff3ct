#!/bin/bash
set -x

source ci/threads.sh

examples=(bootstrap tasks systemc factory)

build_root=build_linux_icpc
function compile {
	mkdir $build_root
	cd $build_root
	cmake .. -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=icpc -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -msse4.2 -Wno-deprecated-declarations -DENABLE_COOL_BASH -std=c++11"
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	make -j $THREADS
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	cd ..
}

source /opt/intel/vars-intel.sh
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