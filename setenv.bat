pushd "%~dp0"
set PKG2=%CD%
if not exist ..\vcpkg_data mkdir ..\vcpkg_data
pushd ..\vcpkg_data
set PKG_INTEMED=%CD%
popd


rem store downloads and binary cache in ..\vcpkg_data
set VCPKG_DOWNLOADS=%PKG_INTEMED%\downloads
set VCPKG_DEFAULT_BINARY_CACHE=%PKG_INTEMED%\archives
if not exist %VCPKG_DEFAULT_BINARY_CACHE% mkdir %VCPKG_DEFAULT_BINARY_CACHE%

rem build some libs as static library
SET VCPKG_DEFAULT_TRIPLET=x64-windows-mixed

rem use my custom ports
set VCPKG_OVERLAY_PORTS=%PKG2%\custom-ports
set VCPKG_OVERLAY_TRIPLETS=%PKG2%\custom-triplets


rem setup for LVWS drive E 
pushd ..\vcpkg
set VCPKG_DOWNLOADS=%CD%\downloads
set VCPKG_ROOT=%CD%
popd
set VCPKG_DEFAULT_BINARY_CACHE=D:\drive_E\binary-cache
set VCPKG_DEFAULT_TRIPLET=x64-windows-static-md
set VCPKG_OVERLAY_PORTS=E:\vcpkg2\custom-ports


SET VCPKG_DEFAULT_TRIPLET=x64-windows-static-md
set VCPKG_OVERLAY_TRIPLETS=
popd
rem now try something like
echo vcpkg install fox-toolkit --editable
echo vcpkg install osg[tools] --editable
