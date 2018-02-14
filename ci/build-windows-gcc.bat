@echo on

set examples=bootstrap tasks factory

set PATH=%PATH%;C:\Program Files\Git\cmd
set PATH=%PATH%;C:\Program Files\CMake\bin
set PATH=%PATH%;C:\Program Files\Git\mingw64\bin

for %%example in (%examples%) do (
	cd %%example
	call:compile
	cd ..
)

:compile
mkdir build
cd build
cmake .. -G"MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++.exe -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -Wno-misleading-indentation  -Wno-deprecated-declarations -funroll-loops -march=native"
mingw32-make
cd ..
EXIT /B 0