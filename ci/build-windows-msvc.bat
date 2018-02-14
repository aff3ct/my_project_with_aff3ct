@echo on

set examples=bootstrap tasks factory

set PATH=%PATH%;C:\Program Files\Git\cmd
set PATH=%PATH%;C:\Program Files\CMake\bin
set PATH=%PATH%;C:\Program Files\Git\mingw64\bin
set "VSCMD_START_DIR=%CD%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

for %%a in (%examples%) do (
	cd %%a
	call :compile
	cd ..
)

exit /B %ERRORLEVEL%

:compile
mkdir build
cd build
cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="-D_CRT_SECURE_NO_DEPRECATE /EHsc /arch:AVX"
devenv /build Release aff3ct.sln
cd ..
exit /B 0