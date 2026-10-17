@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake mangler. Installer CMake fra https://cmake.org/download/ og
  echo Visual Studio Build Tools med "Desktop development with C++".
  pause
  exit /b 1
)
echo Bygger instrumentet - VST3 og/eller CLAP.
cmake -S . -B build-windows -A x64 || goto failed
cmake --build build-windows --config Release --parallel || goto failed
cmake --install build-windows --config Release --component Plugins --prefix "%LOCALAPPDATA%\Programs\Common" || goto failed
echo Plugins installeret i %LOCALAPPDATA%\Programs\Common\VST3 og ...\CLAP.
echo Luk musikprogrammet, aabn det igen, og scan plugins igen.
start "" "build-windows\Instrument_artefacts\Release"
pause
exit /b 0
:failed
echo.
echo Build fejlede. Kopier fejlbeskeden ovenfor, saa vi kan rette den.
pause
exit /b 1
