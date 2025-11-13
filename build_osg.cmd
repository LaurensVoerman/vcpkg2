cd /d "%~dp0"
REM osg                      3.6.5#27         The OpenSceneGraph is an open source high performance 3D graphics toolkit.
REM osg[collada]                              Support for Collada (.dae) files
REM osg[docs]                                 Build OpenSceneGraph reference documentation using doxygen (use: make doc_...
REM osg[examples]                             Enable to build OSG Examples
REM osg[fontconfig]                           Enable Fontconfig support for osgText
REM osg[freetype]                             Enable Freetype support
REM osg[nvtt]                                 Build texture processing tools plugin
REM osg[openexr]                              Build the exr plugin
REM osg[packages]                             Set to ON to generate CPack configuration files and packaging targets
REM osg[plugins]                              Build most OSG plugins
REM osg[sdl1]                                 Build SDL 1 plugin, and enable SDL 1 app features
REM osg[tools]                                Enable to build OSG Applications (e.g. osgviewer)
REM 

call setenv.bat

call vcpkg remove osg
rmdir /s /q E:\vcpkg\buildtrees\osg

REM vcpkg install --recurse osg[collada,docs,examples,fontconfig,freetype,nvtt,openexr,packages,plugins,sdl1,tools]  --editable
REM osg[rest-http-device] is only supported on '!windows',
SET FBX_DIR=E:\osg\3rdparty\extern\FBX\2020.3.7
vcpkg install --recurse osg[collada,default-features,docs,examples,fontconfig,freetype,nvtt,openexr,vnc,lua,fbx,dicom,packages,plugins,sdl1,tools] --editable

pause
E:\osg\36\laurens\vc17_14x64\bin\osgPlugins-3.6.5\_myplugins.cmd