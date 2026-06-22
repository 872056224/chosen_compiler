@echo off
setlocal enabledelayedexpansion

echo ============================================
echo   LL1 Compiler — Windows Build Script
echo ============================================
echo.

REM Detect available generators
set GENERATOR="MinGW Makefiles"
where mingw32-make >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    where make >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo [INFO] MinGW not found, trying Visual Studio...
        where cmake >nul 2>&1
        if %ERRORLEVEL% NEQ 0 (
            echo [ERROR] Neither MinGW nor Visual Studio found.
            echo        Install one of:
            echo          - MinGW-w64: https://www.mingw-w64.org/
            echo          - Visual Studio 2022: https://visualstudio.microsoft.com/
            exit /b 1
        )
        REM Use MSVC (auto-detect latest VS)
        set GENERATOR=Ninja
        where ninja >nul 2>&1
        if %ERRORLEVEL% NEQ 0 (
            cmake --help >nul 2>&1
            for /f "tokens=*" %%g in ('cmake --help ^| findstr /c:"Visual Studio 17"') do (
                set GENERATOR="Visual Studio 17 2022"
            )
            if "!GENERATOR!"=="Ninja" (
                for /f "tokens=*" %%g in ('cmake --help ^| findstr /c:"Visual Studio 16"') do (
                    set GENERATOR="Visual Studio 16 2019"
                )
            )
        )
    )
)

echo [1/4] Configuring with !GENERATOR!...
cmake ..\.. -G !GENERATOR! -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

echo.
echo [2/4] Building compiler...
cmake --build . --config Release -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo [3/4] Running tests...
ctest --output-on-failure
if %ERRORLEVEL% NEQ 0 (
    echo [WARNING] Some tests failed. Check output above.
) else (
    echo [OK] All tests passed.
)

echo.
echo [4/4] Compiler binary:
echo        tools\ll1c\Release\ll1c.exe
echo        tools\ll1c\ll1c.exe
echo.
echo ============================================
echo   Build complete!
echo.
echo   Usage:
echo     ll1c.exe source.ll1 -o output.asm
echo     ll1c.exe source.ll1 --new-codegen --opt -o output.asm
echo ============================================

endlocal
