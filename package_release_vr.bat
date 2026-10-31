@echo off
rem Packages the R.E.A.L. VR compatibility release of the Remastered mod (run build_dsr.bat first).
setlocal
cd /d "%~dp0"
set VERSION=1.9.0-vr1
if not exist build\dsr\dinput8.dll (
    echo build\dsr\dinput8.dll is missing; run build_dsr.bat first.
    exit /b 1
)
if exist build\release-vr rmdir /s /q build\release-vr
mkdir build\release-vr\files
copy /y build\dsr\dinput8.dll build\release-vr\files >nul
copy /y DSR-FPS-Unlock.ini build\release-vr\files >nul
copy /y README.md build\release-vr\files >nul
copy /y LICENSE build\release-vr\files >nul
powershell -NoProfile -Command "Compress-Archive -Path build\release-vr\files\* -DestinationPath build\release-vr\DS1-FPS-Unlock-v%VERSION%-REMASTERED-REALVR.zip"
echo Packaged build\release-vr\DS1-FPS-Unlock-v%VERSION%-REMASTERED-REALVR.zip
