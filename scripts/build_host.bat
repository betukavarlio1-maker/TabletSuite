@echo off
setlocal enabledelayedexpansion
title Windows Host Derleme Betigi

echo [1/4] Derleme ortami kontrol ediliyor...
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [HATA] CMake sistem PATH icerisinde bulunamadi! Lutfen CMake yukleyin.
    pause
    exit /b 1
)

set SCRIPT_DIR=%~dp0
set ROOT_DIR=%SCRIPT_DIR%..
set BUILD_DIR=%ROOT_DIR%\windows_host\build

echo [2/4] CMake yapilandirmasi hazirlaniyor...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [HATA] CMake yapilandirmasi basarisiz oldu!
    pause
    exit /b 1
)

echo [3/4] C++20 Release surumu derleniyor...
cmake --build . --config Release --parallel
if %ERRORLEVEL% neq 0 (
    echo [HATA] Derleme sirasinda hata olustu!
    pause
    exit /b 1
)

echo [4/4] Cikti dosyalari dogrulaniyor...
if exist "%BUILD_DIR%\bin\Release\HostServer.exe" (
    echo [BASARILI] HostServer.exe basariyla olusturuldu: %BUILD_DIR%\bin\Release\HostServer.exe
) else if exist "%BUILD_DIR%\Release\HostServer.exe" (
    echo [BASARILI] HostServer.exe basariyla olusturuldu: %BUILD_DIR%\Release\HostServer.exe
) else (
    echo [BILGI] Derleme tamamlandi.
)
pause
exit /b 0
