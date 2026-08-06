# autoxyq - Virtual HID Keyboard & Mouse Driver

基于 UMDF 2.x 的 Windows 虚拟键鼠驱动程序。

## 构建

需要安装 Windows Driver Kit (WDK) 和 Visual Studio 2022。

### 本地构建

```powershell
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### CI 构建

推送到 main 分支自动触发 GitHub Actions 构建。

## 安装驱动

```powershell
.\scripts\install-driver.ps1
```

## 卸载驱动

```powershell
.\scripts\uninstall-driver.ps1
```
