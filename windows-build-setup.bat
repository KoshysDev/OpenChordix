@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"
cd /d "%ROOT_DIR%" || exit /b 1

if defined NUMBER_OF_PROCESSORS (
    set "JOBS=%NUMBER_OF_PROCESSORS%"
) else (
    set "JOBS=4"
)

set "VCPKG_ROOT=%ROOT_DIR%\external\vcpkg"
set "VCPKG_TRIPLET=x64-mingw-dynamic"
set "VCPKG_HOST_TRIPLET=x64-mingw-dynamic"
set "VCPKG_BINARY_CACHE_DIR=%VCPKG_ROOT%\binary_cache"
if not exist "%VCPKG_BINARY_CACHE_DIR%" mkdir "%VCPKG_BINARY_CACHE_DIR%"
set "VCPKG_DEFAULT_BINARY_CACHE=%VCPKG_BINARY_CACHE_DIR%"
set "VCPKG_BINARY_SOURCES=clear;files,%VCPKG_BINARY_CACHE_DIR%,readwrite"
set "VCPKG_FEATURE_FLAGS=manifests"
set "VCPKG_DEFAULT_TRIPLET=%VCPKG_TRIPLET%"
set "VCPKG_DEFAULT_HOST_TRIPLET=%VCPKG_HOST_TRIPLET%"
set "VCPKG_DISABLE_METRICS=1"

call :need_cmd git
if errorlevel 1 goto :fail
call :need_cmd cmake
if errorlevel 1 goto :fail
call :need_cmd g++
if errorlevel 1 goto :fail
call :detect_make
if errorlevel 1 goto :fail

set "OPENCHORDIX_SHIM_DIR=%TEMP%\openchordix-build-shims"
if not exist "%OPENCHORDIX_SHIM_DIR%" mkdir "%OPENCHORDIX_SHIM_DIR%"
(
    echo @echo off
    echo echo Windows_NT
) > "%OPENCHORDIX_SHIM_DIR%\uname.cmd"
set "PATH=%OPENCHORDIX_SHIM_DIR%;%PATH%"

set "GXX_DIR="
for /f "delims=" %%I in ('where g++ 2^>nul') do (
    if not defined GXX_DIR set "GXX_DIR=%%~dpI"
)

if not defined GXX_DIR (
    echo Unable to locate g++ path.
    goto :fail
)

for %%P in ("%GXX_DIR%\..") do set "MINGW_ROOT=%%~fP"
set "MINGW_POSIX=%MINGW_ROOT:\=/%"
set "BGFX_TOOL_OVERRIDES=CC=%MINGW_POSIX%/bin/x86_64-w64-mingw32-gcc.exe CXX=%MINGW_POSIX%/bin/x86_64-w64-mingw32-g++.exe AR=%MINGW_POSIX%/bin/ar.exe RANLIB=%MINGW_POSIX%/bin/ranlib.exe WINDRES=%MINGW_POSIX%/bin/windres.exe"

if not exist "%MINGW_ROOT%\bin\x86_64-w64-mingw32-g++.exe" (
    echo Missing MinGW compiler: %MINGW_ROOT%\bin\x86_64-w64-mingw32-g++.exe
    echo Ensure your MinGW toolchain is installed and g++ resolves from that toolchain.
    goto :fail
)

if exist ".gitmodules" (
    git submodule update --init --recursive
    if errorlevel 1 goto :fail
) else (
    if not exist "external\bgfx\.git" (
        git clone --depth 1 https://github.com/bkaradzic/bgfx.git external/bgfx
        if errorlevel 1 goto :fail
    )
    if not exist "external\bx\.git" (
        git clone --depth 1 https://github.com/bkaradzic/bx.git external/bx
        if errorlevel 1 goto :fail
    )
    if not exist "external\bimg\.git" (
        git clone --depth 1 https://github.com/bkaradzic/bimg.git external/bimg
        if errorlevel 1 goto :fail
    )
    if not exist "external\vcpkg\.git" (
        git clone --depth 1 https://github.com/microsoft/vcpkg.git external/vcpkg
        if errorlevel 1 goto :fail
    )
)

call "%VCPKG_ROOT%\bootstrap-vcpkg.bat" -disableMetrics
if errorlevel 1 goto :fail

set "BGFX_FILELIST=external\bgfx\3rdparty\dear-imgui\widgets\file_list.inl"
if exist "%BGFX_FILELIST%" (
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
        "$file = '%BGFX_FILELIST%';" ^
        "$content = Get-Content $file -Raw;" ^
        "$old1 = '#if defined(_DIRENT_HAVE_D_TYPE) && defined(DT_DIR)';" ^
        "$old2 = '#if defined(_DIRENT_HAVE_D_TYPE) && defined(DT_DIR) && !defined(__MINGW32__)';" ^
        "$new = '#if defined(_DIRENT_HAVE_D_TYPE) && defined(DT_DIR) && !defined(_WIN32)';" ^
        "$directOld = 'if (item->d_type & DT_DIR)';" ^
        "$directNew = @(" ^
        "'#if !defined(_WIN32) && defined(_DIRENT_HAVE_D_TYPE) && defined(DT_DIR)'," ^
        "'					if (item->d_type & DT_DIR)'," ^
        "'#else'," ^
        "'					struct stat statbuf;'," ^
        "'					if (0 == stat(item->d_name, &statbuf) && (statbuf.st_mode & S_IFDIR) != 0)'," ^
        "'#endif'" ^
        ") -join [Environment]::NewLine;" ^
        "if ($content.Contains($new)) { exit 0 }" ^
        "$patched = $content.Replace($old1, $new).Replace($old2, $new).Replace($directOld, $directNew);" ^
        "if ($patched -ne $content) { Set-Content -Path $file -Value $patched -NoNewline; exit 0 }" ^
        "exit 0"
    if errorlevel 1 goto :fail
)

call %MAKE_CMD% -C external/bgfx .build/projects/gmake-mingw-gcc MINGW=%MINGW_POSIX% SHELL=cmd.exe MAKESHELL=cmd.exe %BGFX_TOOL_OVERRIDES%
if errorlevel 1 goto :fail

call %MAKE_CMD% -C external/bgfx/.build/projects/gmake-mingw-gcc config=release64 bx bimg bimg_decode bgfx MINGW=%MINGW_POSIX% SHELL=cmd.exe MAKESHELL=cmd.exe %BGFX_TOOL_OVERRIDES% -j%JOBS%
if errorlevel 1 goto :fail

call %MAKE_CMD% -C external/bgfx/.build/projects/gmake-mingw-gcc config=release64 shaderc MINGW=%MINGW_POSIX% SHELL=cmd.exe MAKESHELL=cmd.exe %BGFX_TOOL_OVERRIDES% -j%JOBS%
if errorlevel 1 (
    echo Warning: shaderc tool build failed. CMake configure will continue without shader compilation.
)

if exist "build\CMakeCache.txt" del /f /q "build\CMakeCache.txt"
if exist "build\CMakeFiles" rmdir /s /q "build\CMakeFiles"

cmake -B build -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE="%ROOT_DIR%\external\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=%VCPKG_TRIPLET% ^
    -DVCPKG_HOST_TRIPLET=%VCPKG_HOST_TRIPLET%
if errorlevel 1 goto :fail

if not exist "build\CMakeCache.txt" goto :fail

cmake --build build -j%JOBS%
if errorlevel 1 goto :fail

echo Build complete: %ROOT_DIR%\build\src\app\OpenChordix.exe
exit /b 0

:need_cmd
where /q %~1
if errorlevel 1 (
    echo Missing required command: %~1
    exit /b 1
)
exit /b 0

:detect_make
where /q mingw32-make
if not errorlevel 1 (
    set "MAKE_CMD=mingw32-make"
    exit /b 0
)

where /q make
if not errorlevel 1 (
    set "MAKE_CMD=make"
    exit /b 0
)

echo Missing required command: mingw32-make ^(or make from MinGW^)
exit /b 1

:fail
echo Build setup failed.
exit /b 1
