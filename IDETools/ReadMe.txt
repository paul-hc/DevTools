IDETools_vc17 - Visual C++ 2022   [12-Feb-2023]
===============================
Created IDETools_vc17 solution, freshly generated with the App Wizard.
Note: there is no wizard support for automation server interfaces (IDL) and classes (C++).


Automation Server classes:
=========================
Using the Class Wizard in VS 2008:
- Add Automation Server class: IDL and C++ class:
	Right-click on the target project folder, Add > Class.
	Select MFC > MFC Class, press Add.
		Class Name: UserInterface
		Base Class: CCmdTarget
		Automation: (●) Creatable by type ID: IDETools.UserInterface
	Press Finish.

- Add Method/Property to IDL and C++ class:
	Class View: expand IDETools (project) > IDETools (type-library):
	Right-click IUserInterface > Add > Add Method/Property.
	- this will add the new method/property to the IDL file + UserInterface.h and UserInterface.cpp dispatch map

- In case of wizard errors in the script file: C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\VCWizards\1033\common.js
	- Workaround: temporarily replace current base with CCmdTarget


DEBUGGING:
---------
● RunUnitTests (on Windows 64-bit):	=> runs RunUnitTests.vbs script
	Command: RunUnitTests.bat
	Command Arguments: <n/a>
  or:
	Command: $(MyWinSys32Bit)\cscript.exe
	Command Arguments: RunUnitTests.vbs
		- runs CreateObject("IDETools.UserInterface").RunUnitTests()

● Debugging:
	Command: C:\Program Files (x86)\Microsoft Visual Studio\Common\MSDev98\Bin\MSDEV.EXE
		- use Visual Studio 6.0 as debug target - newer VS 2005+ uses a different arhitecture, so it doesn't hit the breakpoints.

● Debug IDETools.dll registration (on Windows 64-bit):
	Command: $(MyWinSys32Bit)\regsvr32.exe
	Command Arguments: $(TargetPath)
		- set breakpoints in the code and start the debugger.


Registration (as Admin):
	regsvr32.exe
	arguments: "$(TargetPath)"



If you specify "\\test-2\q$\lh" the path returned is "\\test-2\q$\lh"
If you specify "\\?\UNC\test-2\q$\lh" the path returned is "\\?\UNC\test-2\q$\lh"
If you specify "U:" the path returned is "U:\"


BSTR CCodeProcessor::ExtractTypeDescriptor( LPCTSTR functionImplLine, LPCTSTR docFileExt )

	Registering IDETools.WorkspaceProfile {0E44AB07-90E1-11D2-A2C9-006097B8DD84}
	Registering IDETools.UserInterface {216EF195-4C10-11D3-A3C8-006097B8DD84}
	Registering IDETools.TextContent {E37FE177-CBB7-11D4-B57C-00D0B74ECB52}
	Registering IDETools.ModuleOptions {4064259A-55DC-4CD5-8A17-DD1CC7B59673}
	Registering IDETools.MenuFilePicker {4DFA7BE2-8484-11D2-A2C3-006097B8DD84}
	Registering IDETools.IncludeFileTree {1006E3E7-1F6F-11D2-A275-006097B8DD84}
	Registering IDETools.FileSearch {C722D0B6-1E2D-11D5-B59B-00D0B74ECB52}
	Registering IDETools.FileLocator {A0580B96-3350-11D5-B5A4-00D0B74ECB52}
	Registering IDETools.FileAccess {1556FB25-22DB-11D2-A278-006097B8DD84}
	Registering IDETools.DspProject {C60E380C-3DE3-4D69-9120-D76A23ECDC8D}
	Registering IDETools.CodeProcessor {F70182C0-AE07-4DEB-AFEB-31BCC6BB244C}


	Registering IDETools.WorkspaceProfile {0E44AB07-90E1-11D2-A2C9-006097B8DD84}
	Registering IDETools.UserInterface {216EF195-4C10-11D3-A3C8-006097B8DD84}
	Registering IDETools.TextContent {E37FE177-CBB7-11D4-B57C-00D0B74ECB52}
	Registering IDETools.ModuleOptions {4064259A-55DC-4CD5-8A17-DD1CC7B59673}
	Registering IDETools.MenuFilePicker {4DFA7BE2-8484-11D2-A2C3-006097B8DD84}
	Registering IDETools.IncludeFileTree {1006E3E7-1F6F-11D2-A275-006097B8DD84}
	Registering IDETools.FileSearch {C722D0B6-1E2D-11D5-B59B-00D0B74ECB52}
	Registering IDETools.FileLocator {A0580B96-3350-11D5-B5A4-00D0B74ECB52}
	Registering IDETools.FileAccess {1556FB25-22DB-11D2-A278-006097B8DD84}
	Registering IDETools.DspProject {C60E380C-3DE3-4D69-9120-D76A23ECDC8D}
	Registering IDETools.CodeProcessor {F70182C0-AE07-4DEB-AFEB-31BCC6BB244C}


CATID_SafeForInitializing  CB5BDC81-93C1-11cf-8F20-00805F2CD064


VC7.1 directories
-----------------
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\7.1
	InstallDir=C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\IDE\

HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\7.1\VC\VC_OBJECTS_PLATFORM_INFO\Win32\Directories
	Include Dirs=$(VCInstallDir)include;$(VCInstallDir)atlmfc\include;$(VCInstallDir)PlatformSDK\include\prerelease;$(VCInstallDir)PlatformSDK\include;$(FrameworkSDKDir)include


Known bugs:
- faulty line split for:
	printf( "\n*** ADD current='%s'   original='%s'\n", currentRecord.getChangeoverPatternKey().c_str(), originalPatternRecord_.getChangeoverPatternKey().c_str() );
	vector<BmCompositeAttribute*>::const_iterator foundStateIt = std::find( flowStates_.begin(), flowStates_.end(), BmCompositeAttribute::BmNameMatcher( rFlowStateName ) );



TODO:
- implement 'Sub ToggleComment()' in C++ code processor
- implement 'Sub ToggleCppEscapeSequences()' in C++ code processor
- implement 'Sub ToggleQuotedString()' in C++ code processor



a get( )
a get( "" )
a get( x, "" )
a get( x, _T("") )
a get( "" )
a get( _T("") )
a get( _T( "" ) )

a get( _T("") ,_T("") ,"" )
a get( _T("")     , _T( "" ) ,""    , /**/     )
a get( _T("")     , _T( "" ) ,""    , /**/, ((char*))get()     )
a get( _T("")     , _T( "" ) ,""    , /**/, (char*)(const char*)"gigel"     )

	/**/  a   get(_T("")  ,_T( "" )  ,  "" , /*,*/ (   char* )(    const char*)"gigel", (CRect&)CRect(1,2,  3,4)     )      // 123
	/**/ a get( _T(""), _T(""), "", /*,*/ (char*)(const char*)"gigel", (CRect&)CRect( 1, 2, 3, 4 ) )      // 123


	static struct { const char* tag; DocLanguage docLanguage; } vsDocLanguages[] =
	{
		{ _T("None"),     DocLang_Cpp },
		{ _T("C/C++"), DocLang_Cpp },
		{ _T("VBS Macro"), DocLang_Basic },
		{ _T("ODBC SQL"), DocLang_SQL },
	};

a
 a
  a
   a
    a


bool operator()( 123, CRect( 123, 456, 789, "abcdef" ), CRect( 123, CRect( 123, 456, CRect( 123, 456, 789, "abcdef" ), "abcdef" ), 789, "abcdef" ), "abcdef" ) const

    a( 1, 2, 3, 4, 5, 6, 7, 8, 9 )      ( a, b ) const
    a( 1, 2, 3, 4 ) const a( 1, 2, 3, 4 ) const a( 1, 2, 3, 4 ) const

	a( 1, 2, 3, 4, 5, 6, 7, 8, 9 )      ( a, b ) const
	a( 1, 2, 3, 4 ) const a( 1, 2, 3, 4 ) const a( 1, 2, 3, 4 ) const

									a( 1, 2, 3, 4 ) const a( 1, 2, 3, 4 ) const a( 1, 2, 3, 4 ) const

template<class _K, class _Ty, class _Pr = less<_K>,	class _A = allocator<_Ty> >	class map
{
};

			CString         implementMethod( const TCHAR* methodPrototype, const TCHAR* templateDecl, int i,
											 const TCHAR* typeQualifier, bool isInline );
			CString			implementMethod( const TCHAR* methodPrototype, const TCHAR* templateDecl, int i,
											 const TCHAR* typeQualifier, bool isInline );

	if ( style == 1; style == 2; style == 3; style == 4 )


	return (int)(char*)"something";
	return (char*)"something";

		CRect( 123, 456, 789, "abcdef" ) const
		CRect( 123, CRect( 123, 456, 789, "abcdef" ), CRect( 123, 456, 789, "abcdef" ), "abcdef" )
		CRect( 123, 456, 789, "abcdef" ), CRect( 123, CRect( 123, 456, 789, "abcdef" ), CRect( 123, 456, 789, "abcdef" ), "abcdef" )


	operator LPCTSTR() const;
	const CString& operator=(const unsigned char* psz);

	const CString&	GetAdditionalIncludePath( const TCHAR* cfg, bool useAnyConfig = true ) const;

/**/ afx_msg int method1( int i, const string& s = _T("") ) const; //xsdtv

	afx_msg int method1( int i, const string& s = _T("") ) const;
	afx_msg int method2( int i, const string& s = _T("") ) const;

	/**/ afx_msg int method2( int i, const string& s = _T(""), CRect r = CRect( 1, 2, "xdstg",   6543 ) ) const; //xsdtv
inline bool Ptr< Type >::operator==( const Ptr< Type >& other ) const
	/**/ afx_msg int method2( int i, const string& s = _T(""), CRect r = CRect( 1, 2, "xdstg",   6543 ) ) const; //xsdtv
inline Ptr< Type >::operator const Type&() const

	/**/ afx_msg int method2( int i, const string& s = _T(""), CRect r = CRect( 1, 2, "xdstg",   6543 ) ) const; //xsdtv
template< typename T, class A >
inline const char*&
CString::Storage::operator = ( const unsigned char* psz ) const

	/**/ afx_msg int method2( int i, const string& s = _T(""), CRect r = CRect( 1, 2, "xdstg",   6543 ) ) const; //xsdtv
template< typename T, class Predicate, int staticIndex > MyClass::Embedded::

template< typename T >
Storage::Storage( void )

     /*asdaed*/      virtual      //sd
virtual    int method( int i ) const;
const char*& CString::Storage::operator=( const unsigned char* psz )
{
}

template< class Type >
inline bool Ptr< Type >::operator==( const Type* _otherPtr ) const
{
	return pointer == _otherPtr;
}


	/**/ afx_msg int method2( int i, const string& name = (string&)::getName(), const string& s = _T(""), CRect r = CRect( 1, 2, "xdstg",   6543 ) ) const throw( CException ); //xsdtv
template< class Type >
void func( int i )
{
}

	/**/ afx_msg int method2( int i, const string& name = (string&)::getName(), const string& s = _T(""), CRect r = CRect( 1, 2, "xdstg",   6543 ) ) const throw( CException ); //xsdtv
// Some comment
// Some comment
// Some comment
// Some comment
/**
	My multi-line comment here
*/
template< class Type >
inline Ptr< Type >::operator const Type*() const
{
	return pointer;
}



	/**/ afx_msg int method2( int i, const string& name = (string&)::getName(), const string& s = _T(""), CRect r = CRect( 1, 2, "xdstg",   6543 ) ) const throw( CException ); //xsdtv
//################# Ptr< Type >::operator const Type* #################
int
Ptr< Type >::method2( int i, const string& name /*= (string&)::getName()*/, const string& s /*= _T("")*/,
					  CRect r /*= CRect( 1, 2, "xdstg", 6543 )*/ ) const
{
}


	/**/ afx_msg int method2( int i, const string& name = (string&)::getName(), const string& s = _T(""), CRect r = CRect( 1, 2, "xdstg",   6543 ) ) const throw( CException ); //xsdtv
/**
	My multi-line comment here
*/
template< class Type >
bool Ptr< Type >::Embedded::operator()( const Type* _otherPtr ) const
{
	// TODO...
	return /**/;
}

//################# Ptr< Type >::operator-> #################
template< class Type >
inline const Type* Ptr::Embedded::operator->() const
{
	ASSERT( pointer != NULL );
	return pointer;
}


							CppImplementationFormatter( const CFormatterOptions& _options );
							~CppImplementationFormatter();
CppImplementationFormatter::

	/**/ virtual bool operator()( const Type* _otherPtr ) const
template<>
inline const Type* Ptr< T, C >::Embedded::operator->() const
{
	ASSERT( pointer != NULL );
	return pointer;
}


HTREEITEM CPP::firstThat( CondFunc condFunc,
						  HTREEITEM hStartItem = TVI_ROOT,
						  /* ROOT */ bool forward = true,
						  /* FORWARD */ char* args = (char*)(const char*)::getString(),
						  /**/bool forceBinding = false ) const throw( CException,
																	   CFileException,
																	   COleException )
#
HTREEITEM BASIC::firstThat( CondFunc condFunc,
					HTREEITEM hStartItem = TVI_ROOT,    ' ROOT
				bool forward = true,' FORWARD
			 char* args = (   char* )(    const char*)::getString(),
bool forceBinding = false ) const   throw( CException, CFileException, COleException )
#
HTREEITEM BASIC::firstThat( CondFunc condFunc, HTREEITEM hStartItem = TVI_ROOT, bool forward = true, char* args = (char*)(const char*)::getString(),
							bool forceBinding = false ) throw( CException, CFileException, COleException )
#
HTREEITEM CFileTreeDialog::firstThat( CondFunc condFunc, const CRect& rect1 = CRect( 12, 123, max( 1234, getMaxHeight( "height1", "width1" ) ), 12345 ), const CRect& rect2 = CRect( 20, 24, max( 235, getMaxHeight( "height2", "width2" ) ), 23456 ), void* args = NULL, bool forceBinding = false )

HTREEITEM firstThat( CondFunc condFunc, const CRect& rect1 = CRect( 12, 123, max( 1234, getMaxHeight( "height1", "width1" ) ), 12345 ), const CRect& rect2 = CRect( 20, 24, max( 235, getMaxHeight( "height2", "width2" ) ), 23456 ), void* args = NULL, bool forceBinding = false )

HTREEITEM firstThat( CondFunc condFunc,
		 const CRect& rect1 = CRect( 12, 123, max( 1234, getMaxHeight( "height1", "width1" ) ), 12345 ),
	const CRect& rect2 = CRect( 20, 24, max( 235, getMaxHeight( "height2", "width2" ) ), 23456 ),
			 void* args = NULL, bool forceBinding = false )

HTREEITEM firstThat( (CondFunc)myCondFunc, TVI_ROOT, (const char*)getName(), getSize( 125, "some string parameter" ), true );

	if ( firstThat( (CondFunc)myCondFunc, vector< string >, (const char*)getName(), getSize( 125, "some string parameter" ), true ) )

	for (int i=0;i<=len&&i!=0;++i)
	for ( int i = 0; i < len; ++i )
	for (int i=0;i<len>;++i)
	for (int i=0;i!=len;++i)
	for ( int i = 0; i < len; ++i )

	for (vector<CBraceRule>::const_iterator itBrace=m_braceRules.begin(); itBrace!=m_braceRules.end();++itBrace)     ;
	for (  vector<   CBraceRule   >::const_iterator   itBrace   =  m_braceRules.begin()  ; itBrace  !=  m_braceRules.end() ;  ++itBrace  )  ;
	for ( vector< CBraceRule >::const_iterator itBrace = m_braceRules.begin(); itBrace != m_braceRules.end(); ++itBrace );
		if ( ( *itBrace ).m_isArgList )
			argListBraces += ( *itBrace ).m_braceOpen;

//################# SectionParser::extractSection #################
// Parse the line, search for section names, and if found it appends it as a line to
bool SectionParser::extractSection( LPCTSTR line, LPCTSTR sectionFilter /*= NULL*/,
									str::CaseType caseType /*= str::IgnoreCase*/, LPCTSTR sep /*= _T("\r\n")*/ )
{
	int				tagStartPos = -1, tagLength = 0;
	CString			sectionCore = extractTag( line, tagStartPos, tagLength );

	if ( sectionCore.IsEmpty() || sectionCore.CompareNoCase( tagCoreEOS ) == 0 )
		return false;
	// If section name filter is specified -> check for filter match:
	if ( sectionFilter != NULL && *sectionFilter != 0 )
		if ( !str::findStringPos( sectionCore, sectionFilter, caseType ).IsValid() )
			return false;

	if ( !textContent.IsEmpty() )
		textContent += ( CString( sep ) + sectionCore );
	return true;
}


inline const TokenRange& TokenRange::operator()( const TokenRange& source ) const
{
	return *this;
}

inline const TokenRange& TokenRange::operator[]( int index ) const
{
	return *this;
}

inline bool TokenRange::operator!=( const TokenRange& rightToken ) const
{
	return !operator==( rightToken );
}


//------------------------------------------------------------------------------
// ReadMe
//------------------------------------------------------------------------------


	ASSERT( false );
	{
		int foundPos = 0;
		foundPos = str::findStringPos( _T("123ab45ab678abc90"), _T("ab"), foundPos ).second;
		foundPos = str::findStringPos( _T("123ab45ab678abc90"), _T("ab"), foundPos ).second;
		foundPos = str::findStringPos( _T("123ab45ab678abc90"), _T("ab"), foundPos ).second;
	}

	{
		int foundPos = -1;
		foundPos = str::reverseFindStringPos( _T("123ab45ab678abc90"), _T("ab"), foundPos ).first;
		foundPos = str::reverseFindStringPos( _T("123ab45ab678abc90"), _T("ab"), foundPos ).first;
		foundPos = str::reverseFindStringPos( _T("123ab45ab678abc90"), _T("ab"), foundPos ).first;
	}




private sub ListProjectFiles()
	dim dspProject

	set dspProject = CreateObject( "IDETools.DspProject" )

	dspProject.ClearAllFileFilters
	dspProject.AddFileFilter "*.h"
	dspProject.AddFileFilter "*.hxx"
	dspProject.AddFileFilter "*.hpp"
	dspProject.AddFileFilter "*.inl"
	dspProject.AddFileFilter "*.t"
	dspProject.AddFileFilter "*.c"
	dspProject.AddFileFilter "*.cpp"
	dspProject.AddFileFilter "*.cxx"
	dspProject.AddFileFilter "*.idl"
	dspProject.AddFileFilter "*.odl"
	dspProject.AddFileFilter "*.rc"
	dspProject.AddFileFilter "*.rc2"

	for each project In Application.Projects
		PrintToOutputWindow project.Name & " -> " & project.FullName
		if project.Type = "Build" then
			dspProject.DspProjectFilePath = project.FullName

			dim index, filesCount

			filesCount = dspProject.GetNewSourceFileCount()
			if filesCount > 0 then
				PrintToOutputWindow "  *NEW FILES:"
				for index = 0 to filesCount - 1
					PrintToOutputWindow "   " & dspProject.GetNewSourceFileAt( index )
				next
			end if

			filesCount = dspProject.GetRemovedSourceFileCount()
			if filesCount > 0 then
				PrintToOutputWindow "  *REMOVED FILES:"
				for index = 0 to filesCount - 1
					PrintToOutputWindow "   " & dspProject.GetRemovedSourceFileAt( index )
				next
			end if
		end if
	next

	set dspProject = nothing
end sub



			for ( itFile = m_projectFiles.begin(); itFile != m_projectFiles.end(); ++itFile )
				TRACE( _T("^^ '%s'\n"), (LPCTSTR)( *itFile ).fullPath );
			for ( itFolder = sourceFolders.begin(); itFolder != sourceFolders.end(); ++itFolder )
				TRACE( _T("** '%s'\n"), (LPCTSTR)*itFolder );

========================================================================
    MICROSOFT FOUNDATION CLASS LIBRARY : IDETools Project Overview
========================================================================


AppWizard has created this IDETools DLL for you.  This DLL not only
demonstrates the basics of using the Microsoft Foundation classes but
is also a starting point for writing your DLL.

This file contains a summary of what you will find in each of the files that
make up your IDETools DLL.

IDETools.vcproj
    This is the main project file for VC++ projects generated using an Application Wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

Application.h
    This is the main header file for the DLL.  It declares the
    CApplication class.

IDETools.cpp
    This is the main DLL source file.  It contains the class CApplication.
    It also contains the OLE entry points required of inproc servers.

IDETools.idl
    This file contains the Object Description Language source code for the
    type library of your DLL.

IDETools.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
    Visual C++.

res\IDETools.rc2
    This file contains resources that are not edited by Microsoft
    Visual C++.  You should place all resources not editable by
    the resource editor in this file.

IDETools.def
    This file contains information about the DLL that must be
    provided to run with Microsoft Windows.  It defines parameters
    such as the name and description of the DLL.  It also exports
    functions from the DLL.

IDETools.clw (VC6 only)
    This file contains information used by ClassWizard to edit existing
    classes or add new classes.  ClassWizard also uses this file to store
    information needed to create and edit message maps and dialog data
    maps and to create prototype member functions.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named IDETools.pch and a precompiled types file named StdAfx.obj.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Visual C++ reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////
