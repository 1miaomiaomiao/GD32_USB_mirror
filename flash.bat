@echo off
chcp 65001 >nul
REM GD32H759IM one-click flash (pyOCD + GD-Link, under-reset connect + chip erase)
REM Flashes the ELF (carries its own load address) with the GD32H7 DFP pack passed explicitly.

set PYOCD_DIR=D:\RT-ThreadStudio\repo\Extract\Debugger_Support_Packages\RealThread\PyOCD\0.2.9
set PACK=D:\RT-ThreadStudio\repo\Extract\Debugger_Support_Packages\RealThread\PyOCD\0.2.9\packs\GigaDevice.GD32H7xx_DFP.1.4.0.pack
set TARGET=gd32h759im
set ELF=D:\RT-ThreadStudio\workspace\GD32\Debug\rtthread.elf

cd /d %PYOCD_DIR%

echo Flashing %ELF% (chip erase + program, under-reset) ...
pyocd.exe flash --pack="%PACK%" --target=%TARGET% -M under-reset --erase=chip --frequency=1000000 "%ELF%"
if errorlevel 1 (
    echo.
    echo *** Flash failed: make sure GD-Link is connected and the board is powered.
    echo *** If No ACK persists, pull BOOT0 high, re-power, then run this script again.
    pause
    exit /b 1
)

echo.
echo === Done. Reset or re-power the board to run the program. ===
pause
