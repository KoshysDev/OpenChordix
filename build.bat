@echo off
setlocal ENABLEDELAYEDEXPANSION

set "REPO_ROOT=%~dp0"
set "EXTERNAL_DIR=%REPO_ROOT%external"
set "VCPKG_DIR=%EXTERNAL_DIR%\vcpkg"
set "CMAKE_DIR=%EXTERNAL_DIR%\cmake"
set "CMAKE_EXE="

if not exist "%EXTERNAL_DIR%" mkdir "%EXTERNAL_DIR%"

REM vcpkg clone/bootstrap if missing
if not exist "%VCPKG_DIR%\.git" (
  echo [INFO] Cloning vcpkg...
  pushd "%EXTERNAL_DIR%"
  git clone --depth 1 https://github.com/microsoft/vcpkg vcpkg || (echo [ERROR] vcpkg clone failed & exit /b 1)
  popd
)
if not exist "%VCPKG_DIR%\vcpkg.exe" (
  echo [INFO] Bootstrapping vcpkg...
  call "%VCPKG_DIR%\bootstrap-vcpkg.bat" || (echo [ERROR] vcpkg bootstrap failed & exit /b 1)
)

REM find or fetch portable CMake
for /f "delims=" %%i in ('where cmake 2^>nul') do set "CMAKE_EXE=%%i"
if not defined CMAKE_EXE (
  set "CMAKE_ZIP_URL=https://github.com/Kitware/CMake/releases/download/v3.29.6/cmake-3.29.6-windows-x86_64.zip"
  set "CMAKE_ZIP=%EXTERNAL_DIR%\cmake.zip"
  set "CMAKE_EXE=%CMAKE_DIR%\bin\cmake.exe"
  if not exist "%CMAKE_EXE%" (
    echo [INFO] Downloading portable CMake...
    powershell -NoProfile -Command "Invoke-WebRequest -UseBasicParsing '%CMAKE_ZIP_URL%' -OutFile '%CMAKE_ZIP%'" || (echo [ERROR] CMake download failed & exit /b 1)
    echo [INFO] Extracting CMake...
    powershell -NoProfile -Command "Remove-Item -Recurse -Force '%CMAKE_DIR%' -ErrorAction Ignore; Expand-Archive -Path '%CMAKE_ZIP%' -DestinationPath '%EXTERNAL_DIR%' -Force; $d=Get-ChildItem '%EXTERNAL_DIR%' -Directory | Where-Object { $_.Name -like 'cmake-*' } | Select-Object -First 1; Move-Item $d.FullName '%CMAKE_DIR%' -Force" || (echo [ERROR] CMake unzip failed & exit /b 1)
    del "%CMAKE_ZIP%" >nul 2>&1
  )
)
if not exist "%CMAKE_EXE%" ( echo [ERROR] cmake.exe not found & exit /b 1 )

REM clean old build dir
if exist "%REPO_ROOT%build" rmdir /s /q "%REPO_ROOT%build"

echo [INFO] Configuring via preset...
"%CMAKE_EXE%" --preset mingw-vcpkg || (echo [ERROR] Configure failed & exit /b 1)

echo [INFO] Building via preset...
"%CMAKE_EXE%" --build --preset mingw-vcpkg-release || (echo [ERROR] Build failed & exit /b 1)

REM copy DLL
set "BIN_SRC=%REPO_ROOT%build\vcpkg_installed\x64-mingw-dynamic\bin"
for /r "%REPO_ROOT%build" %%f in (OpenChordix.exe) do set "OUT_DIR=%%~dpf"
if exist "%BIN_SRC%" if defined OUT_DIR (
  echo [INFO] Copying DLLs to %OUT_DIR%
  xcopy /y /q "%BIN_SRC%\*.dll" "%OUT_DIR%" >nul
)

echo [OK] Build finished.
endlocal
