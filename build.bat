@echo off
set project_dir=%~dp0
set configuration=MinSizeRel
if not defined DevEnvDir (
	call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
)
cd %project_dir%
@echo on
msbuild build\xrig.sln /p:Configuration=%configuration%
@echo off
IF %ERRORLEVEL% NEQ 0 (
	EXIT /B %ERRORLEVEL%
)
xcopy libuv-1.14.1-x64\libuv.dll build\%configuration%\ /y 1> NUL
xcopy devcon\devcon.exe build\%configuration%\ /y 1> NUL
(
echo @echo off
echo cd %%~dp0
echo xrig.exe
echo pause
) > build\%configuration%\run.bat
(
echo @echo off
echo cd %%~dp0
echo devcon.exe disable "PCI\VEN_1002&DEV_687F"
echo timeout /t 3
echo devcon.exe enable "PCI\VEN_1002&DEV_687F"
echo timeout /t 3
echo xrig.exe
echo pause
) > build\%configuration%\run_with_reset.bat
