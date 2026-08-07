# install-driver.ps1 — 安装 autoxyq 虚拟 HID 驱动
# 需要管理员权限

param(
    [string]$InfPath = ".\autoxyq-drv.inx"
)

$ErrorActionPreference = "Stop"

# 检查管理员权限
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "This script requires Administrator privileges. Restarting with elevation..."
    Start-Process powershell -Verb RunAs -ArgumentList "-File `"$PSCommandPath`" -InfPath `"$InfPath`""
    exit
}

Write-Host "=== autoxyq Driver Installation ==="

$infFullPath = Resolve-Path $InfPath -ErrorAction Stop
Write-Host "INF: $infFullPath"

# 安装驱动
Write-Host "Installing driver..."
pnputil /add-driver "$infFullPath" /install

Write-Host "Driver installation complete."
Write-Host "Verify: Open Device Manager and look for 'autoxyq Virtual HID Device' under 'Human Interface Devices'"
