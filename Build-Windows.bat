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
where git >nul 2>nul
if errorlevel 1 (
  echo Git mangler. Installer Git fra https://git-scm.com/download/win
  pause
  exit /b 1
)
echo Bygger Artilody Plugweaver og plugin-motoren - C++ / JUCE 7.0.12 / GPLv3.
cmake -S . -B build-windows -A x64 -DBUILD_TESTING=ON || goto failed
cmake --build build-windows --config Release --parallel || goto failed
ctest --test-dir build-windows -C Release --output-on-failure || goto failed
echo Faerdig: build-windows\Plugweaver_artefacts\Release\Artilody Plugweaver.exe
start "" "build-windows\Plugweaver_artefacts\Release\Artilody Plugweaver.exe"
exit /b 0
:failed
echo.
echo Build fejlede. Kopier fejlbeskeden ovenfor, saa vi kan rette den.
pause
exit /b 1
