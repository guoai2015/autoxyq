# uninstall-driver.ps1 — 卸载 autoxyq 虚拟 HID 驱动
# 需要管理员权限

param(
    [string]$DriverName = "autoxyq-drv.inf"
)

$ErrorActionPreference = "Stop"

if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Start-Process powershell -Verb RunAs -ArgumentList "-File `"$PSCommandPath`" -DriverName `"$DriverName`""
    exit
}

Write-Host "=== autoxyq Driver Uninstallation ==="

# 查找已安装的驱动
$installedDriver = pnputil /enum-drivers | Select-String $DriverName

if ($installedDriver) {
    Write-Host "Removing driver..."
    pnputil /delete-driver $DriverName /uninstall
    Write-Host "Driver removed."
} else {
    Write-Host "Driver not found (already uninstalled?)."
}
