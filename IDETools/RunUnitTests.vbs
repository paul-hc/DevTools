' Usage Windows 64-bit:
'	%MyWinSys32Bit%\cscript.exe RunUnitTests.vbs
'	C:\Windows\SysWOW64\cscript.exe RunUnitTests.vbs
'
' Debug in Visual C++ 2008/2022:
'	Right-click on IDETools > Properties > Debugging:
'		Command:			$(MyWinSys32Bit)\cscript.exe
'		Command Arguments:	RunUnitTests.vbs
'
'	For Debugging IDETools.dll
'		Original Command:	C:\Program Files (x86)\Microsoft Visual Studio\Common\MSDev98\Bin\MSDEV.EXE

Sub RunIdeToolsTests()
	'DESCRIPTION: Runs the unit tests defined in IDETools.dll
	Set uiObject = CreateObject( "IDETools.UserInterface" )

	uiObject.RunUnitTests()
	Set uiObject = Nothing
End Sub


Call RunIdeToolsTests()
