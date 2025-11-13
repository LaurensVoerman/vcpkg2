rem mod needed to get coin config working..for osgdb_iv
REM E:\vcpkg\installed\x64-windows-static-md\share\Coin\coin-config.cmake
		REM find_dependency(Boost MODULE)



rem a dirty hack to get osg collada plugin working
if exist E:\vcpkg\installed\x64-windows-static-md\debug\lib\collada-dom2.5-dp-vc140-mtX.lib goto :SKIP1
move E:\vcpkg\installed\x64-windows-static-md\debug\lib\collada-dom2.5-dp-vc140-mt.lib E:\vcpkg\installed\x64-windows-static-md\debug\lib\collada-dom2.5-dp-vc140-mtX.lib
:SKIP1

lib /OUT:E:\vcpkg\installed\x64-windows-static-md\debug\lib\collada-dom2.5-dp-vc140-mt.lib E:\vcpkg\buildtrees\collada-dom\x64-windows-static-md-dbg\dom\src\1.4\colladadom141.lib E:\vcpkg\buildtrees\collada-dom\x64-windows-static-md-dbg\dom\src\1.5\colladadom150.lib E:\vcpkg\installed\x64-windows-static-md\debug\lib\uriparser.lib E:\vcpkg\buildtrees\collada-dom\x64-windows-static-md-dbg\dom\collada-dom2.5-dp-vc140-mt.lib 
rem release version
if exist E:\vcpkg\installed\x64-windows-static-md\lib\collada-dom2.5-dp-vc140-mtX.lib goto :SKIP2
move E:\vcpkg\installed\x64-windows-static-md\lib\collada-dom2.5-dp-vc140-mt.lib E:\vcpkg\installed\x64-windows-static-md\lib\collada-dom2.5-dp-vc140-mtX.lib
:SKIP2
lib /OUT:E:\vcpkg\installed\x64-windows-static-md\lib\collada-dom2.5-dp-vc140-mt.lib E:\vcpkg\buildtrees\collada-dom\x64-windows-static-md-rel\dom\src\1.4\colladadom141.lib E:\vcpkg\buildtrees\collada-dom\x64-windows-static-md-rel\dom\src\1.5\colladadom150.lib E:\vcpkg\installed\x64-windows-static-md\lib\uriparser.lib E:\vcpkg\buildtrees\collada-dom\x64-windows-static-md-rel\dom\collada-dom2.5-dp-vc140-mt.lib 
