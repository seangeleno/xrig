@echo off
set project_dir=%~dp0
mkdir build 2> NUL
cd build
@echo on
cmake .. -G "Visual Studio 15 2017 Win64" -DUV_INCLUDE_DIR="..\\libuv-1.14.1-x64\\include" -DUV_LIBRARY="..\\libuv-1.14.1-x64\\libuv.lib" -DMHD_INCLUDE_DIR="..\\libmicrohttpd-0.9.58-w32-bin\\x86_64\\VS2017\\Release-static" -DMHD_LIBRARY="..\\libmicrohttpd-0.9.58-w32-bin\\x86_64\\VS2017\\Release-static\\libmicrohttpd.lib"
@echo off
IF %ERRORLEVEL% NEQ 0 (
	cd %project_dir%
	EXIT /B %ERRORLEVEL%
)
cd %project_dir%