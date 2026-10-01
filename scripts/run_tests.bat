@echo off
setlocal enabledelayedexpansion
title 100 Senaryolu Test Kosucusu

set SCRIPT_DIR=%~dp0
set ROOT_DIR=%SCRIPT_DIR%..
set TEST_BUILD_DIR=%ROOT_DIR%\tests\build

echo [1/3] Test projesi yapilandiriliyor...
if not exist "%TEST_BUILD_DIR%" mkdir "%TEST_BUILD_DIR%"
cd /d "%TEST_BUILD_DIR%"

cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [HATA] Test CMake yapilandirmasi basarisiz!
    pause
    exit /b 1
)

echo [2/3] Test paketi derleniyor...
cmake --build . --config Release --parallel
if %ERRORLEVEL% neq 0 (
    echo [HATA] Test binary derlenemedi!
    pause
    exit /b 1
)

echo [3/3] 100 Senaryolu Otomasyon Testi Calistiriliyor...
echo.
if exist "%TEST_BUILD_DIR%\Release\TestSuiteRunner.exe" (
    "%TEST_BUILD_DIR%\Release\TestSuiteRunner.exe"
) else if exist "%TEST_BUILD_DIR%\TestSuiteRunner.exe" (
    "%TEST_BUILD_DIR%\TestSuiteRunner.exe"
) else (
    echo [HATA] TestSuiteRunner.exe bulunamadi!
    pause
    exit /b 1
)

echo.
echo Test oturumu tamamlandi.
pause
exit /b 0
