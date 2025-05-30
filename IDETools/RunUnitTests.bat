@echo off

if "%1"=="64bit" (
	C:\Windows\SysWOW64\cscript.exe RunUnitTests.vbs
	echo Completed ^(64-bit^) unit tests!
) else (
	%MyWinSys32Bit%\cscript.exe RunUnitTests.vbs
	echo Completed ^(32-bit^) unit tests!
)

:: pause
