@echo on

set examples=bootstrap tasks factory pipeline

cd examples
for %%a in (%examples%) do (
	cd %%a
	call :compile
	if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
	cd ..
)

exit /B %ERRORLEVEL%

:compile
mkdir build_windows_gcc
cd build_windows_gcc
cmake .. -G"MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++.exe -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -Wno-misleading-indentation -Wno-deprecated-declarations -funroll-loops -mavx"
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
mingw32-make -j %THREADS%
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
cd ..
exit /B 0