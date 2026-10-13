# install_winusb.ps1 -- bind WinUSB to VID_28E9&PID_028F (run ELEVATED).

$ErrorActionPreference = "Continue"
$root = "D:\RT-ThreadStudio\workspace\GD32\tools"
$log  = Join-Path $root "install_winusb.log"
$inf  = Join-Path $root "rtthread_winusb.inf"
$hwid = "VID_28E9&PID_028F"

function Log($m) { $m | Tee-Object -FilePath $log -Append }

Set-Content -Path $log -Value "=== WinUSB bind $(Get-Date) ==="

$dev = Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match $hwid }
if ($dev) {
    Log "Removing cached device node: $($dev.InstanceId)"
    pnputil /remove-device "$($dev.InstanceId)" 2>&1 | Out-String | Log
    Start-Sleep -Seconds 2
} else {
    Log "No present device node found before rescan."
}

Log "Scanning for hardware changes ..."
pnputil /scan-devices 2>&1 | Out-String | Log
Start-Sleep -Seconds 6

$dev = Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match $hwid }
if ($dev) {
    $svc  = (Get-PnpDeviceProperty -InstanceId $dev.InstanceId -KeyName 'DEVPKEY_Device_Service' -ErrorAction SilentlyContinue).Data
    $prob = (Get-PnpDeviceProperty -InstanceId $dev.InstanceId -KeyName 'DEVPKEY_Device_ProblemCode' -ErrorAction SilentlyContinue).Data
    Log "After rescan: Status=$($dev.Status) Service=[$svc] Problem=$prob"
    if ($dev.Status -eq 'OK' -and "$svc" -eq 'WinUSB') {
        Log "SUCCESS: WinUSB auto-bound via MS OS descriptors (no INF needed)."
        exit 0
    }
} else {
    Log "Device did not reappear after rescan."
}

Log "Installing fallback INF: $inf"
pnputil /add-driver "$inf" /install 2>&1 | Out-String | Log
Start-Sleep -Seconds 4

$dev = Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match $hwid }
if ($dev) {
    $svc  = (Get-PnpDeviceProperty -InstanceId $dev.InstanceId -KeyName 'DEVPKEY_Device_Service' -ErrorAction SilentlyContinue).Data
    $prob = (Get-PnpDeviceProperty -InstanceId $dev.InstanceId -KeyName 'DEVPKEY_Device_ProblemCode' -ErrorAction SilentlyContinue).Data
    Log "After INF install: Status=$($dev.Status) Service=[$svc] Problem=$prob"
    if ($dev.Status -eq 'OK' -and "$svc" -eq 'WinUSB') {
        Log "SUCCESS: WinUSB bound via INF."
        exit 0
    }
    Log "STILL NOT BOUND -- try: pnputil /scan-devices, or unplug/replug the board."
    exit 1
}
Log "DEVICE NOT FOUND after INF install -- check USB cable / port."
exit 1
