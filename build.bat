@echo off
rem 命令示例： .\build.bat vdagent x64
rem 命令示例： .\build.bat vdservice x64
rem 命令示例： .\build.bat vdagent Win32
rem 命令示例： .\build.bat vdservice Win32

rem 设置 VS2022 的环境变量
setlocal enabledelayedexpansion

rem 使用 vswhere 查找 VS2022 安装路径
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set VS2022INSTALLPATH=%%i
)

if not exist "!VS2022INSTALLPATH!" (
    echo 未找到 Visual Studio 2022 安装路径，请确认是否已安装。
    exit /b 1
)

rem 设置 devenv 路径
set _devenv="!VS2022INSTALLPATH!\Common7\IDE\devenv.com"

rem 调用 vcvarsall.bat 设置编译环境
call "!VS2022INSTALLPATH!\VC\Auxiliary\Build\vcvarsall.bat" %2

rem 配置并行编译的核心数
set CPUCount=4

rem 检查是否提供了主工程名称和目标平台作为参数
if "%~1"=="" (
    echo 请提供主工程名称作为第一个参数。
    echo 示例:build.bat Service x64
    exit /b 1
)

if "%~2"=="" (
    echo 请提供目标平台x64 或 Win32作为第二个参数。
    echo 示例:build.bat Service x64
    exit /b 1
)

rem 使用传入的参数编译解决方案
del BuildLog.txt
echo 正在编译项目: %~1,目标平台: %~2
%_devenv% Sample-Test.sln /Build "Debug|%~2" /Project "%~1" /Out "BuildLog.txt"
if errorlevel 1 (
    echo 编译失败，请查看 BuildLog.txt 获取详细信息。
    exit /b 1
)
