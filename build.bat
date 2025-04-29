@echo off
rem ����ʾ���� .\build.bat vdagent x64
rem ����ʾ���� .\build.bat vdservice x64
rem ����ʾ���� .\build.bat vdagent Win32
rem ����ʾ���� .\build.bat vdservice Win32

rem ���� VS2022 �Ļ�������
setlocal enabledelayedexpansion

rem ʹ�� vswhere ���� VS2022 ��װ·��
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set VS2022INSTALLPATH=%%i
)

if not exist "!VS2022INSTALLPATH!" (
    echo δ�ҵ� Visual Studio 2022 ��װ·������ȷ���Ƿ��Ѱ�װ��
    exit /b 1
)

rem ���� devenv ·��
set _devenv="!VS2022INSTALLPATH!\Common7\IDE\devenv.com"

rem ���� vcvarsall.bat ���ñ��뻷��
call "!VS2022INSTALLPATH!\VC\Auxiliary\Build\vcvarsall.bat" %2

rem ���ò��б���ĺ�����
set CPUCount=4

rem ����Ƿ��ṩ�����������ƺ�Ŀ��ƽ̨��Ϊ����
if "%~1"=="" (
    echo ���ṩ������������Ϊ��һ��������
    echo ʾ��:build.bat Service x64
    exit /b 1
)

if "%~2"=="" (
    echo ���ṩĿ��ƽ̨x64 �� Win32��Ϊ�ڶ���������
    echo ʾ��:build.bat Service x64
    exit /b 1
)

rem ʹ�ô���Ĳ�������������
del BuildLog.txt
echo ���ڱ�����Ŀ: %~1,Ŀ��ƽ̨: %~2
%_devenv% Sample-Test.sln /Build "Debug|%~2" /Project "%~1" /Out "BuildLog.txt"
if errorlevel 1 (
    echo ����ʧ�ܣ���鿴 BuildLog.txt ��ȡ��ϸ��Ϣ��
    exit /b 1
)
