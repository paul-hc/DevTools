New Solution Creation Steps (in VC 2008):
===========================
1) Generated a new Visual C++ > Win32 Console Application:
	Application type: Console application	[default]
	Add common header files for: MFC [X]
	Additional options: Precompiled header	[default]
2) Move the generated solution files to wiz_gen\ subdirectory.
3) Copy all files from TransferFiles\ solution to StampBuildVersion\ directory, and:
- Rename:
	TransferFiles.sln		StampBuildVersion.sln
	TransferFiles.vcproj	StampBuildVersion.vcproj
- Search and replace in StampBuildVersion\.sln,.vcproj the original ProjectGUID with the newly generated ProjectGUID="{F8FC3A4A-D533-41CE-8AEC-FFDFBC98F327}".
- Remove in StampBuildVersion.vcproj under section Name="VCLinkerTool" the entries OutputFile="$(OutDir)\xFer.exe" - this defaults output file to StampBuildVersion.exe
4) Build and run the new solution, which inherits the project dependency to UTL_BASE and standard precompiled headers.
5) DONE! Do initial commit to Git. Start customizing the new project.

6) Remove dependency to MFC library, as it will not not used:
Select StampBuildVersion project in Solution Explorer, choose Properties:
	select Configuration=All Configurations
	select Configuration Properties > General, and change:
		Use of MFC=Use Standard Windows Libraries
* Note: UTL_BASE actually links to MFC statically, so MFC dependencies are properly resolved when linking,


HOW TO USE:
==========
1) In a Visual C++ project with GUI that has the VS_VERSION_INFO resource in the .rc file, add the "BuildTimestamp" line to:
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "BuildTimestamp", "-"

2) Select the project in Solution Explorer, right click "Properties":
	- select Configuration=ReleaseU
		(it's best to avoid Configuration=All Configurations to minimize .rc file updates in debug builds)
	- select Configuration Properties > Build Events > Pre-Build Event and assign:
		Command Line: StampBuildVersion.exe $(ProjectDir)$(ProjectName).rc
		Description: Stamping build time in resource file...
	- click OK.

Now the project will embed a "BuildTimestamp" value in version info, that will get updated to the latest build time on each new build.
Open the About Box (F1) to check the "Built on: " field is displayed (will be hidden if the .rc file has no "BuildTimestamp" entry).


========================================================================
    CONSOLE APPLICATION : StampBuildVersion Project Overview
========================================================================

AppWizard has created this StampBuildVersion application for you.

This file contains a summary of what you will find in each of the files that
make up your StampBuildVersion application.


StampBuildVersion.vcproj
    This is the main project file for VC++ projects generated using an Application Wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

StampBuildVersion.cpp
    This is the main application source file.

/////////////////////////////////////////////////////////////////////////////
AppWizard has created the following resources:

StampBuildVersion.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
    Visual C++.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Visual C++ reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named StampBuildVersion.pch and a precompiled types file named StdAfx.obj.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////
