set GIT="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd\git.exe"

rem need disk space... 37,6 GB  = 40.428.019.712 bytes

if not exist c:\dev (
  FOR /F "usebackq tokens=3" %%s IN (`DIR C:\ /-C /-O /W`) DO (
	IF %%s LSS 40428019712 (
	   echo Insufficient free space; need some 38 GB
	   exit /B 1
	)
  )

  mkdir c:\dev
)

cd /d c:\dev

if not exist vcpkg (
  REM %GIT% clone https://github.com/LaurensVoerman/vcpkg.git
  %GIT% clone https://github.com/microsoft/vcpkg.git
) else (
  cd vcpkg
  %GIT% pull
  cd ..
)
if not exist vcpkg\vcpkg.exe (
  cd vcpkg
  CALL bootstrap-vcpkg.bat -disableMetrics
  cd ..
)

if not exist vcpkg2 (
  %GIT% clone https://github.com/LaurensVoerman/vcpkg2.git
) else (
  cd vcpkg2
  %GIT% pull
  cd ..
)
cd vcpkg2
call setenv.bat

call vcpkg.bat install osg[tools,collada] --recurse
rem vcpkg search osg --featurepackages
rem vcpkg install osg[*]
cd ..
pause
