@echo on

set examples=bootstrap tasks factory

set PATH=%PATH%;C:\Program Files\Git\cmd
set PATH=%PATH%;C:\Program Files\CMake\bin
set PATH=%PATH%;C:\Program Files\Git\mingw64\bin
set "VSCMD_START_DIR=%CD%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

cd examples
for %%a in (%examples%) do (
	cd %%a
	call :compile
	if %ERRORLEVEL% neq 0 exit /B %ERRORLEVEL%
	cd ..
)

exit /B %ERRORLEVEL%

:compile
mkdir build_windows_msvc
cd build_windows_msvc
cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="-D_CRT_SECURE_NO_DEPRECATE /EHsc /arch:AVX"
if %ERRORLEVEL% neq 0 exit /B %ERRORLEVEL%
devenv /build Release my_project.sln
if %ERRORLEVEL% neq 0 exit /B %ERRORLEVEL%
cd ..
exit /B 0