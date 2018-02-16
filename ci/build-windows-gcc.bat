@echo on

set examples=bootstrap tasks factory

set PATH=%PATH%;C:\Program Files\Git\cmd
set PATH=%PATH%;C:\Program Files\CMake\bin
set PATH=%PATH%;C:\Program Files\Git\mingw64\bin

cd examples
for %%a in (%examples%) do (
	cd %%a
	call :compile
	cd ..
)

exit /B %ERRORLEVEL%

:compile
mkdir build_windows_gcc
cd build_windows_gcc
cmake .. -G"MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++.exe -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -Wno-misleading-indentation  -Wno-deprecated-declarations -funroll-loops -march=native"
if %ERRORLEVEL% neq 0 exit /B %ERRORLEVEL%
mingw32-make
if %ERRORLEVEL% neq 0 exit /B %ERRORLEVEL%
cd ..
exit /B 0