@echo off
setlocal

echo ============================================
echo   Chess Engine - Windows Build Script
echo ============================================
echo.

:: Check if g++ is available
where g++ >nul 2>&1
if errorlevel 1 (
    echo ERROR: g++ not found. Please install MinGW-w64 and add it to your PATH.
    echo Download: https://www.mingw-w64.org/ or via MSYS2: https://www.msys2.org/
    pause
    exit /b 1
)

echo g++ found:
g++ --version | findstr /C:"g++"
echo.

:: Set output name
set TARGET=chess_engine.exe
set TEST_TARGET=perft_test.exe
set FLAGS=-std=c++17 -O3 -Wall -Iengine

:: Source files
set SRCS=engine\main.cpp engine\board\Board.cpp engine\movegen\MoveGen.cpp engine\eval\Eval.cpp engine\search\Search.cpp engine\cli\CLI.cpp engine\util\PGN.cpp
set TEST_SRCS=tests\perft_test.cpp engine\board\Board.cpp engine\movegen\MoveGen.cpp

:: Parse arguments
if "%1"=="test" goto build_test
if "%1"=="clean" goto clean
goto build_main

:build_main
echo Building %TARGET%...
g++ %FLAGS% -o %TARGET% %SRCS%
if errorlevel 1 (
    echo.
    echo BUILD FAILED.
    pause
    exit /b 1
)
echo.
echo Build successful! Run with: %TARGET%
echo.
if "%1"=="" pause
goto end

:build_test
echo Building perft tests...
g++ %FLAGS% -o %TEST_TARGET% %TEST_SRCS%
if errorlevel 1 (
    echo.
    echo TEST BUILD FAILED.
    pause
    exit /b 1
)
echo Running tests...
echo.
%TEST_TARGET%
pause
goto end

:clean
echo Cleaning build artifacts...
if exist %TARGET% del /f %TARGET%
if exist %TEST_TARGET% del /f %TEST_TARGET%
echo Done.
goto end

:end
endlocal
