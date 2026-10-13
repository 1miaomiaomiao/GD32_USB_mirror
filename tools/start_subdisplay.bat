@echo off
chcp 65001 >nul
REM GD32 USB secondary display -- host-side launcher. Run once and leave it open;
REM the board menu switches modes afterwards. Auto-reconnects across replugs.

cd /d "%~dp0.."

echo USB secondary display host tool (board-controlled).
echo Leave this window open. Use the panel menu to switch modes.
echo Ctrl-C here to stop.
echo.

python -u tools\usb2screen.py

echo.
echo Ended (or python not found). Install deps once with:
echo     pip install pyusb libusb psutil pillow numpy mss
pause
