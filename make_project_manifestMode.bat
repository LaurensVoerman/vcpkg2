if "%CD%\#"=="%~dp0#" GOTO :RUN
if "%HJKL%\#"=="HJKL#" GOTO :RUN
set HJKL=1
cmd /K "cd /d "%~dp0" && "%~dp0setenv.bat" && %~nx0"
goto :EOF
:RUN

pushd ..\vcpkg
set VCPKG_DOWNLOADS=%CD%\downloads
set VCPKG_ROOT=%CD%
popd

set VCPKG_DEFAULT_BINARY_CACHE=D:\drive_E\binary-cache
set VCPKG_DEFAULT_TRIPLET=x64-windows-static-md
set VCPKG_OVERLAY_PORTS=E:\vcpkg2\custom-ports

cd project
..\vcpkg.bat install
