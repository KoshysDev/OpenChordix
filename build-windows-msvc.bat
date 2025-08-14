@echo off
setlocal enabledelayedexpansion

REM ------------------------------------------------------------------
REM OpenChordix — Windows build (MSVC + Ninja + vcpkg)
REM This script:
REM   1) Ensures Ninja is available (winget install if missing)
REM   2) Clones + bootstraps vcpkg into external\vcpkg (if missing)
REM   3) Enters VS 2022 Build Tools environment
REM   4) Configures with CMake (Ninja + vcpkg toolchain) and builds
REM ------------------------------------------------------------------

REM ---- 0) Move to repo root
pushd "%~dp0..\"
set REPO_ROOT=%CD%

echo.
echo === OpenChordix Windows Build (MSVC + Ninja) ===
echo Repo: %REPO_ROOT%
echo.

REM ---- 1) Ensure Ninja is installed
where ninja >NUL 2>&1
if errorlevel 1 (
  echo [INFO] Ninja not found. Attempting installation via winget...
  where winget >NUL 2>&1
  if errorlevel 1 (
    echo [ERROR] winget is not available. Please install Ninja manually and re-run.
    echo Download: https://github.com/ninja-build/ninja/releases
    exit /b 1
  )
  winget install Ninja-build.Ninja --source winget --silent
  if errorlevel 1 (
    echo [ERROR] Failed to install Ninja via winget. Install manually and re-run.
    exit /b 1
  )
)
for /f "usebackq delims=" %%P in (`where ninja`) do set NINJA_EXE=%%P
echo [OK] Ninja: %NINJA_EXE%

REM ---- 2) Ensure vcpkg exists in external\vcpkg
set VCPKG_DIR=%REPO_ROOT%\external\vcpkg
if not exist "%VCPKG_DIR%\.git" (
  echo [INFO] Cloning vcpkg into external\vcpkg ...
  mkdir "%REPO_ROOT%\external" 2>NUL
  git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
  if errorlevel 1 (
    echo [ERROR] Failed to clone vcpkg.
    exit /b 1
  )
)

REM ---- 3) Bootstrap vcpkg
if not exist "%VCPKG_DIR%\vcpkg.exe" (
  echo [INFO] Bootstrapping vcpkg ...
  call "%VCPKG_DIR%\bootstrap-vcpkg.bat"
  if errorlevel 1 (
    echo [ERROR] vcpkg bootstrap failed.
    exit /b 1
  )
)

REM ---- 4) Enter VS 2022 Build Tools environment (amd64)
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
  echo [ERROR] vswhere not found. Install Visual Studio Build Tools 2022.
  exit /b 1
)

for /f "usebackq delims=" %%I in (`%VSWHERE% -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do set VS_PATH=%%I
if "%VS_PATH%"=="" (
  echo [ERROR] Visual Studio Build Tools 2022 not found.
  exit /b 1
)

call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64
if errorlevel 1 (
  echo [ERROR] Failed to enter VS dev environment.
  exit /b 1
)

REM ---- 5) Make sure our Ninja is preferred on PATH
set NIN_DIR=%NINJA_EXE:\ninja.exe=%
set PATH=%NIN_DIR%;%PATH%

REM ---- 6) vcpkg environment knobs
set VCPKG_ROOT=%VCPKG_DIR%
set VCPKG_FORCE_SYSTEM_BINARIES=1
set VCPKG_NINJA=%NINJA_EXE%
REM in case yasm droping compile errors, uncomment this:
REM set VCPKG_CMAKE_CONFIGURE_OPTIONS=-DCMAKE_MAKE_PROGRAM=%NINJA_EXE%;-DCMAKE_POLICY_DEFAULT_CMP0026=OLD

REM ---- 7) Configure + build with CMake
set BDIR=%REPO_ROOT%\build-ninja
if exist "%BDIR%" rmdir /s /q "%BDIR%"
mkdir "%BDIR%"

echo.
echo [CONFIGURE] CMake (Ninja + vcpkg toolchain) -> %BDIR%
cmake -G Ninja -S "%REPO_ROOT%" -B "%BDIR%" ^
  -D CMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  -D VCPKG_TARGET_TRIPLET=x64-windows ^
  -D CMAKE_MAKE_PROGRAM="%NINJA_EXE%"
if errorlevel 1 (
  echo [ERROR] CMake configure failed.
  exit /b 1
)

echo.
echo [BUILD] OpenChordix
cmake --build "%BDIR%"
if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

echo.
echo [SUCCESS] Build finished.
echo  - EXE: %BDIR%\src\app\OpenChordix.exe
echo.
popd
exit /b 0