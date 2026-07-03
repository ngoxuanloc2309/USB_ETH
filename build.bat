@echo off
setlocal

set TOOLCHAIN=cmake/gcc-arm-none-eabi.cmake
set JOBS=8

if "%1"=="build" goto build
if "%1"=="debug" goto debug
if "%1"=="clean" goto clean

echo Usage:
echo   build.bat build   - configure (if needed) + build Release
echo   build.bat debug   - configure (if needed) + build Debug
echo   build.bat clean   - clean build directories
goto end

:build
if not exist build\release\build.ninja (
    echo Configuring Release...
    echo  -----    -       -    ---     -
    echo |           -   -      -  -    -
    echo  -----        -        -   -   -
    echo       |       -        -    -  -
    echo  -----        -        -     ---
    cmake -S . -B build\release -G Ninja ^
        -DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN% ^
        -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 goto error
)
echo Building Release...
cmake --build build\release -j%JOBS%
goto end

:debug
if not exist build\debug\build.ninja (
    echo Configuring Debug...
    cmake -S . -B build\debug -G Ninja ^
        -DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN% ^
        -DCMAKE_BUILD_TYPE=Debug
    if errorlevel 1 goto error
)
echo Building Debug...
cmake --build build\debug -j%JOBS%
goto end

:clean
echo Cleaning...
if exist build\release cmake --build build\release --target clean 2>nul
if exist build\debug   cmake --build build\debug   --target clean 2>nul
goto end

:error
echo Configure failed!
exit /b 1

:end
endlocal