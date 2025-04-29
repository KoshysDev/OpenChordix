@echo off
setlocal

REM Set paths (adjust if needed)
set VCPKG_ROOT=%~dp0vcpkg
set VCPKG_TRIPLET=x64-mingw-dynamic
set TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

REM Ensure vcpkg is bootstrapped
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo [ERROR] vcpkg.exe not found in %VCPKG_ROOT%
    echo Please install or clone vcpkg in the same folder and run bootstrap-vcpkg.bat.
    exit /b 1
)

REM Clean previous build (optional)
rmdir /s /q build 2>nul

REM Configure CMake
echo [INFO] Configuring project...
cmake -S . -B build -G "MinGW Makefiles" ^
    -DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN% ^
    -DVCPKG_TARGET_TRIPLET=%VCPKG_TRIPLET% ^
    -DCMAKE_CXX_COMPILER=C:/ProgramData/mingw64/mingw64/bin/g++.exe

if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

REM Build project
echo [INFO] Building project...
cmake --build build --config Release

if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

endlocal
