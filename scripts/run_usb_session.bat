@echo off
setlocal enabledelayedexpansion
title USB Oturumu ve ADB Tunelleme Baslatici

echo ========================================================
echo ENTERPRISE TABLET SUITE: USB OTURUM YONETICISI
echo ========================================================

echo [1/4] ADB baglantisi kontrol ediliyor...
where adb >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [UYARI] ADB komutu bulunamadi. Android SDK Platform-Tools'u PATH'e ekleyin.
    echo Baglanti Wi-Fi veya dogrudan IP uzerinden denenecektir.
    goto START_SERVER
)

adb get-state >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [UYARI] Bagli bir Android cihaz tespit edilemedi.
    echo Cihazi USB ile baglayip USB Hata Ayiklamayi acin.
    goto START_SERVER
)

echo [2/4] ADB Port Yonlendirmesi (Reverse Tunnel) kuruluyor...
adb reverse tcp:8080 tcp:8080
if %ERRORLEVEL% equ 0 (
    echo [BILGI] Port 8080 basariyla yonlendirildi: Tablet -> PC
) else (
    echo [UYARI] ADB reverse basarisiz oldu. Wi-Fi soketi kullanilacak.
)

:START_SERVER
echo [3/4] Host Server binary tespiti yapiliyor...
set SCRIPT_DIR=%~dp0
set ROOT_DIR=%SCRIPT_DIR%..
set HOST_EXE=

if exist "%ROOT_DIR%\windows_host\build\bin\Release\HostServer.exe" (
    set HOST_EXE=%ROOT_DIR%\windows_host\build\bin\Release\HostServer.exe
) else if exist "%ROOT_DIR%\windows_host\build\Release\HostServer.exe" (
    set HOST_EXE=%ROOT_DIR%\windows_host\build\Release\HostServer.exe
)

if "%HOST_EXE%"=="" (
    echo [HATA] HostServer.exe bulunamadi! Lutfen once build_host.bat betigini calistirin.
    pause
    exit /b 1
)

echo [4/4] Sunucu Yonetici Yetkileriyle Baslatiliyor: %HOST_EXE%
echo.
"%HOST_EXE%"
pause
exit /b 0
