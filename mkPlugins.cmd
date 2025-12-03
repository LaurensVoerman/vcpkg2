cd /d "%~dp0"
rmdir /s /q E:\vcpkg\buildtrees\osgplugins
rem vcpkg install osgplugins --head --editable 
vcpkg install osgplugins --editable 
