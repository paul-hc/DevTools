' Usage Windows 64-bit:
'	C:\Windows\SysWOW64\cscript RunUnitTests.vbs
'
' Debug in Visual C++ 2022:
'	Right-click on IDETools > Properties > Debugging:
'		Command: C:\Windows\SysWOW64\cscript.exe
'		Command Arguments: $(ProjectDir)RunUnitTests.vbs
'
'			Original Command: C:\Program Files (x86)\Microsoft Visual Studio\Common\MSDev98\Bin\MSDEV.EXE

Sub RunIdeToolsTests()
	'DESCRIPTION: Runs the unit tests defined in IDETools.dll
	Set uiObject = CreateObject( "IDETools.UserInterface" )

	uiObject.RunUnitTests()
	Set uiObject = Nothing
End Sub


Call RunIdeToolsTests()
