@echo on

set examples=bootstrap tasks factory

set "VSCMD_START_DIR=%CD%"
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

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
cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="-D_CRT_SECURE_NO_DEPRECATE /EHsc /MP%THREADS% /arch:AVX"
if %ERRORLEVEL% neq 0 exit /B %ERRORLEVEL%
devenv /build Release my_project.sln
rem msbuild my_project.sln /t:Build /p:Configuration=Release
if %ERRORLEVEL% neq 0 exit /B %ERRORLEVEL%
cd ..
exit /B 0