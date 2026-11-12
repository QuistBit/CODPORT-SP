@echo off
setlocal
rem PATCH40a: caller-independent paths; propagate generator failures to MSBuild.
if not exist "%~dp0extractfuncs.exe" (
    echo ERROR: Build the extractfuncs project before generating the game function table.
    exit /b 1
)
pushd "%~dp0..\game"
if errorlevel 1 exit /b 1
"%~dp0extractfuncs.exe" *.c
set "cod1_extract_result=%errorlevel%"
popd
exit /b %cod1_extract_result%
